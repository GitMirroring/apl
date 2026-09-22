/*
    This file is part of GNU APL, a free implementation of the
    ISO/IEC Standard 13751, "Programming Language APL, Extended"

    Copyright © 2008-2026  Dr. Jürgen Sauermann

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/** @file
*/

// This file provides an API to functions that differ on different platforms


#ifndef SYS_HH_DEFINED
#define SYS_HH_DEFINED

#include <cstdint>
#include <cstdio>
#include <cstring>

#include <semaphore.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>

#include "config.h"   // for HAVE_SYS_MMAN_H

#if HAVE_SYS_MMAN_H
# include <sys/mman.h>   // must be at file scope: its extern "C" block
                          // cannot appear inside a class body
#endif

#include "Common.hh"   // for CERR and InterruptContext

//════════════════════════════════════════════════════════════════════════════
/// A FILE * that pclose()s itself FILE * when destructed.
class PipePointer
{
public:
   /// @param fptr the pipe FILE pointer to manage
   PipePointer(FILE * fptr) : ptr(fptr) {}
   ~PipePointer()   { close(); }

   FILE * operator->()
      { return ptr; }

   int close()
      {
        if (ptr)
           {
             const int ret = pclose(ptr);
             ptr = 0;
             return ret;
           }

        return 0;
      }

protected:
   FILE * ptr;
};

class Sys
{
public:

#if HAVE_SYS_MMAN_H

   /// @param fd open file descriptor to map
   /// @param len number of bytes to map
   static inline const uint8_t * mmap(int fd, int len)
      {
      // ::mmap, not mmap: an unqualified call here would resolve to
      // this very function (Sys::mmap hides the global ::mmap once
      // found, regardless of the mismatched argument count) -- silent
      // infinite recursion, not a call to the POSIX syscall. This
      // branch was dead code until the HAVE_MMAN_H/HAVE_SYS_MMAN_H
      // macro-name mismatch below was fixed, so it had never been
      // compiled; both this and the equivalent bug in munmap() below
      // were caught only then.
      const void * vp = ::mmap(NULL, len, PROT_READ, MAP_PRIVATE, fd, 0);
         if (vp == MAP_FAILED)   return 0;
         return reinterpret_cast<const uint8_t *>(vp);
      }

   /// @param data pointer returned by mmap()
   /// @param len number of bytes originally mapped
   static inline void munmap(const uint8_t * data, int len)
      { ::munmap(const_cast<void *>(reinterpret_cast<const void *>(data)),
                len); }

#else // ! HAVE_SYS_MMAN_H ===================================================

   /// @param fd open file descriptor to read
   /// @param len number of bytes to read
   static inline const uint8_t * mmap(int fd, size_t len)
      {
         if (uint8_t * buffer = new uint8_t[len + 1])
            {
              buffer[len] = 0;   // for strXXX()
              if (int(len) == read(fd, buffer, len))   return buffer;

              delete[] buffer;
            }

         return 0;
      }

   /// @param data pointer returned by mmap()
   /// @param len number of bytes originally allocated (unused on this platform)
   static inline void munmap(const uint8_t * data, size_t len)
      { delete [] data; }

#endif // ! HAVE_SYS_MMAN_H

   /// invasive memory test
   /// @param base start address of the memory region to probe
   /// @param blocks number of 64-bit blocks to probe
   /// @param verbosity level of diagnostic output
   static int64_t probe_memory(uint64_t * base, uint64_t blocks, int verbosity);

   /// outcome of sem_wait_safe(): why the wait ended.
   enum Wait_result
      {
        WAIT_OK = 0,     ///< the semaphore was actually posted
        WAIT_CTRL_C,     ///< the user hit ^C while waiting
        WAIT_TIMEOUT,    ///< timeout_seconds elapsed unposted
        WAIT_ERROR,      ///< sem_timedwait() itself failed unexpectedly
      };

   /// sem_wait() that (a) retries on EINTR, except for the user's own ^C,
   /// and (b) gives up after timeout_seconds instead of blocking forever
   /// if the wait is never satisfied at all. A plain sem_wait() returns
   /// prematurely (with the semaphore left un-posted) if a signal arrives
   /// while blocked, regardless of what that signal is about; a premature,
   /// signal-triggered return silently reintroduces exactly the race the
   /// wait exists to prevent unless the caller checks sem_wait()'s return
   /// value, which most callers don't. Incidental signals are therefore
   /// retried transparently -- but if the wait is never satisfied at all
   /// (a genuine deadlock or lost wakeup on the other side), blindly
   /// retrying forever would make that hang un-interruptible, which is
   /// worse than the original race: the user's own ^C (GNU APL installs a
   /// real SIGINT handler, see main.cc) must still be able to break out,
   /// so EINTR caused by an already-raised attention ends the wait instead
   /// of being retried -- and so must a wait that is simply never going
   /// to be satisfied.
   ///
   /// This function may be called from threads other than the interpreter
   /// thread (e.g. a GUI driver's own callback/event-loop threads), so it
   /// must never raise a C++/APL exception itself (Workspace/SI access is
   /// not safe from those threads). It returns WAIT_OK if the semaphore
   /// was actually posted, or a specific WAIT_xxx reason otherwise --
   /// logged to CERR either way since that is safe from any thread.
   /// Callers running on the interpreter thread that want a proper APL
   /// error on failure should catch the non-WAIT_OK outcomes themselves
   /// and raise their own error (see Quad_PLOT::sem_wait_safe_I() for an
   /// example).
   /// @param sema the semaphore to wait for
   /// @param what human-readable description of what is being waited
   ///        for, used only in the CERR diagnostic printed on failure
   /// @param timeout_seconds how long to wait before giving up
#if HAVE_SEM_TIMEDWAIT

   static inline Wait_result
   sem_wait_safe(sem_t * sema, const char * what, int timeout_seconds)
      {
        struct timespec deadline;
        clock_gettime(CLOCK_REALTIME, &deadline);
        deadline.tv_sec += timeout_seconds;

        for (;;)
           {
             if (sem_timedwait(sema, &deadline) == 0)   return WAIT_OK;

             if (errno == EINTR)
                {
                  if (InterruptContext::attention_is_raised())
                     {
                       CERR << "*** Sys::sem_wait_safe(): ^C hit while "
                               "waiting in sem_timedwait() for " << what
                            << endl;
                       return WAIT_CTRL_C;
                     }
                  continue;
                }

             if (errno == ETIMEDOUT)
                {
                  CERR << "*** Sys::sem_wait_safe(): timed out after "
                       << timeout_seconds << "s waiting for " << what
                       << endl;
                  return WAIT_TIMEOUT;
                }

             CERR << "*** Sys::sem_wait_safe(): sem_timedwait() while "
                     "waiting for " << what << " failed unexpectedly: "
                  << strerror(errno) << endl;
             return WAIT_ERROR;
           }
      }

#else // !HAVE_SEM_TIMEDWAIT -- e.g. macOS: Darwin's <semaphore.h> declares
      // sem_wait()/sem_trywait() for unnamed semaphores but never
      // sem_timedwait() at all (a compile-time absence, not the runtime
      // one HAVE_SEM_INIT works around). Poll sem_trywait() against a
      // deadline computed the same way, sleeping briefly between polls
      // instead of blocking natively in the kernel; the ^C/timeout/error
      // outcomes and their CERR wording are unchanged.

   static inline Wait_result
   sem_wait_safe(sem_t * sema, const char * what, int timeout_seconds)
      {
      enum { POLL_NANOSECONDS = 10 * 1000 * 1000 };   // 10ms between polls

        struct timespec deadline;
        clock_gettime(CLOCK_REALTIME, &deadline);
        deadline.tv_sec += timeout_seconds;

        for (;;)
           {
             if (sem_trywait(sema) == 0)   return WAIT_OK;

             if (errno == EINTR)   continue;   // a plain, unrelated signal

             if (errno != EAGAIN)
                {
                  CERR << "*** Sys::sem_wait_safe(): sem_trywait() while "
                          "waiting for " << what << " failed unexpectedly: "
                       << strerror(errno) << endl;
                  return WAIT_ERROR;
                }

             if (InterruptContext::attention_is_raised())
                {
                  CERR << "*** Sys::sem_wait_safe(): ^C hit while waiting "
                          "in sem_trywait() for " << what << endl;
                  return WAIT_CTRL_C;
                }

             struct timespec now;
             clock_gettime(CLOCK_REALTIME, &now);
             if (now.tv_sec > deadline.tv_sec ||
                 (now.tv_sec == deadline.tv_sec &&
                  now.tv_nsec >= deadline.tv_nsec))
                {
                  CERR << "*** Sys::sem_wait_safe(): timed out after "
                       << timeout_seconds << "s waiting for " << what
                       << endl;
                  return WAIT_TIMEOUT;
                }

             struct timespec poll_delay = { 0, POLL_NANOSECONDS };
             nanosleep(&poll_delay, 0);
           }
      }

#endif // HAVE_SEM_TIMEDWAIT
};
//────────────────────────────────────────────────────────────────────────────
/// open a pipe for reading or writing
/// @param command shell command string to execute
/// @param mode open mode passed to popen() (e.g. "r" or "w")
/// number of sys_popen() streams currently open without a matching
/// sys_pclose() yet (Bugs30 #19). A function-local static in an inline
/// function, not a plain file-scope static: this header can be included
/// by more than one .cc file, and every one of them must nest against
/// the same single, program-wide count.
inline int & sys_popen_nesting()
{
   static int nesting = 0;
   return nesting;
}
//────────────────────────────────────────────────────────────────────────────
inline FILE * sys_popen(const char * command, const char * mode)
{
   // Restore SIGCHLD to its default disposition BEFORE popen() forks the
   // child, not after. sys_pclose() below leaves SIGCHLD as SIG_IGN at
   // rest (elsewhere in the interpreter that relies on children being
   // auto-reaped without an explicit wait()); popen()'s own child can
   // exit before control ever reaches a signal() call placed after
   // popen() returns, and while SIGCHLD is still SIG_IGN, POSIX/Linux
   // auto-reaps an exiting child the instant it exits. If that race is
   // hit, pclose()'s internal waitpid() later finds no child left to
   // wait for, fails with ECHILD, and pclose() returns -1 instead of the
   // real wait status -- confirmed via a Bugs27 #59(g) regression report
   // (Bill Heagy) on a slow single-core 32-bit machine, where a `)HOST
   // exit 3` reliably printed -1 instead of 3. signal() takes effect
   // synchronously in this thread, so doing it first closes the window
   // entirely: the child can never be forked while SIGCHLD is IGN.
   //
   // Only on the *first* concurrently-open stream (Bugs30 #19): with two
   // streams open at once, unconditionally flipping SIGCHLD back to
   // SIG_IGN in sys_pclose() below as soon as the FIRST one closes
   // auto-reaps the SECOND stream's still-running child the instant it
   // exits, so that stream's own later pclose() finds no child left
   // (waitpid -> ECHILD) and wrongly returns -1 instead of the real wait
   // status. Nesting the disposition change keeps SIGCHLD at SIG_DFL for
   // as long as ANY sys_popen() stream is still open.
   //
#if ! MINGW_SRC
   if (sys_popen_nesting()++ == 0)   signal(SIGCHLD, SIG_DFL);
#endif // ! MINGW_SRC

FILE * file = popen(command, mode);
   return file;
}
//────────────────────────────────────────────────────────────────────────────
/// close a pipe opened with sys_popen()
/// @param stream FILE pointer returned by sys_popen()
inline int sys_pclose(FILE *stream)
{
const int ret = pclose(stream);

   // Back to SIG_IGN at rest only once the LAST concurrently-open
   // sys_popen() stream closes (Bugs30 #19) -- see sys_popen() above.
#if ! MINGW_SRC
   if (--sys_popen_nesting() == 0)   signal(SIGCHLD, SIG_IGN);
#endif // ! MINGW_SRC

   return ret;
}
//════════════════════════════════════════════════════════════════════════════
/// A MAII compliant wrapper for popen()/pclose()
class PipeReader
{
public:
   /// default constructor for placement new
   PipeReader() : fp(0) {}


   /// constructor
   /// @param command shell command to open as a readable pipe
   PipeReader(const char * command)
      {
        fp = sys_popen(command, "r");
      }

   /// destructor: pclose fp (unless already closed).
   ~PipeReader()
      {
        if (fp)   sys_pclose(fp);
      }

   int fgetc() const
      {
        if (fp)   return ::fgetc(fp);
        return EOF;
      }

   /// @param buffer destination buffer for the line
   /// @param buflen capacity of buffer in bytes
   char * fgets(char * buffer, int buflen) const
      {
        if (fp)   return ::fgets(buffer, buflen, fp);
        return 0;
      }

   /// @param buffer destination buffer for the data
   /// @param buflen maximum number of bytes to read
   size_t fread(char * buffer, int buflen) const
      {
        if (fp)   return ::fread(buffer, 1, buflen, fp);
        return 0;
      }

   /// return \b true if ptr is not open
   bool operator !() const
      { return fp == 0; }

   /// return \b true if ptr is (open)
   bool operator +() const
      { return fp != 0; }

   int close()
      {
       if (fp)
          {
            const int ret = sys_pclose(fp);
            fp = 0;
            return ret;
          }
        return 0;
      }

protected:
   FILE * fp;
};
//════════════════════════════════════════════════════════════════════════════
/// A MAII compliant wrapper for fopen()/fclose()
class FileReader
{
public:
   /// default constructor for placement new
   FileReader() : fp(0) {}


   /// constructor: from alrady open FILE *
   /// @param file already-open FILE pointer to manage
   FileReader(FILE * file)
      {
        fp = file;
      }

   /// constructor: from open fd
   /// @param fd open file descriptor to wrap
   FileReader(int fd)
      {
        fp = fdopen(fd, "r");
      }

   /// constructor: from filename
   /// @param filename path of the file to open for reading
   FileReader(const char * filename)
      {
        fp = fopen(filename, "r");
      }

   /// destructor: pclose fp (unless already closed).
   ~FileReader()
      {
        if (fp)   fclose(fp);
      }

   int fgetc() const
      {
        if (fp)   return ::fgetc(fp);
        return EOF;
      }

   /// @param buffer destination buffer for the line
   /// @param buflen capacity of buffer in bytes
   char * fgets(char * buffer, int buflen) const
      {
        if (fp)   return ::fgets(buffer, buflen, fp);
        return 0;
      }

   /// @param buffer destination buffer for the data
   /// @param buflen maximum number of bytes to read
   size_t fread(char * buffer, int buflen) const
      {
        if (fp)   return ::fread(buffer, 1, buflen, fp);
        return 0;
      }

   FILE * get_FILE() const
      { return fp; }

   /// return \b true if ptr is not open
   bool operator !() const
      { return fp == 0; }

   /// return \b true if ptr is (open)
   bool operator +() const
      { return fp != 0; }

protected:
   FILE * fp;
};
//════════════════════════════════════════════════════════════════════════════
/// A MAII compliant wrapper for fopen()/fclose()
class FileWriter
{
public:
   /// default constructor for placement new
   FileWriter() : fp(0) {}


   /// constructor: from alrady open FILE *
   /// @param file already-open FILE pointer to manage
   FileWriter(FILE * file)
      {
        fp = file;
      }

   /// constructor: from open fd
   /// @param fd open file descriptor to wrap
   /// @param mode fopen-style open mode string
   FileWriter(int fd, const char * mode = "w")
      {
        fp = fdopen(fd, mode);
      }

   /// constructor: from filename
   /// @param filename path of the file to open for writing
   /// @param mode fopen-style open mode string
   FileWriter(const char * filename, const char * mode = "w")
      {
        fp = fopen(filename, mode);
      }

   /// destructor: pclose fp (unless already closed).
   ~FileWriter()
      {
        if (fp)   fclose(fp);
      }

   FILE * get_FILE() const
      { return fp; }

   /// return \b true if ptr is not open
   bool operator !() const
      { return fp == 0; }

   /// return \b true if ptr is (open)
   bool operator +() const
      { return fp != 0; }

   /// @param buffer source data to write
   /// @param bufsize number of bytes to write
   size_t fwrite(const void * buffer, size_t bufsize)
      { if (fp)   return ::fwrite(buffer, 1, bufsize, fp);
        return 0;
      }

protected:
   FILE * fp;
};
//════════════════════════════════════════════════════════════════════════════

//════════════════════════════════════════════════════════════════════════════
// Platform-independent wrappers for socket option functions.
// On POSIX, setsockopt()/getsockopt() take const void * / void * for optval.
// On Windows (winsock2) the same argument is const char * / char *.

#if HAVE_SYS_SOCKET_H
#  include <sys/socket.h>
#endif

#if HAVE_WINSOCK2_H
#  include <winsock2.h>
#  include <ws2tcpip.h>
#endif

#if HAVE_SYS_SOCKET_H || HAVE_WINSOCK2_H

/// platform-independent setsockopt()
inline int
sys_setsockopt(int fd, int level, int optname,
               const void * optval, socklen_t optlen)
{
#if HAVE_WINSOCK2_H
   return setsockopt(fd, level, optname,
                     reinterpret_cast<const char *>(optval), optlen);
#else
   return setsockopt(fd, level, optname, optval, optlen);
#endif
}

/// platform-independent getsockopt()
inline int
sys_getsockopt(int fd, int level, int optname,
               void * optval, socklen_t * optlen)
{
#if HAVE_WINSOCK2_H
   return getsockopt(fd, level, optname,
                     reinterpret_cast<char *>(optval), optlen);
#else
   return getsockopt(fd, level, optname, optval, optlen);
#endif
}

#endif // HAVE_SYS_SOCKET_H || HAVE_WINSOCK2_H
//════════════════════════════════════════════════════════════════════════════

#endif // SYS_HH_DEFINED

