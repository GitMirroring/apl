/*
    This file is part of GNU APL, a free implementation of the
    ISO/IEC Standard 13751, "Programming Language APL, Extended"

    Copyright © 2018-2026  Dr. Jürgen Sauermann

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

#ifndef __Quad_PLOT_DEFINED__
#define __Quad_PLOT_DEFINED__

#include <errno.h>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <time.h>

#include "Common.hh"
#include "QuadFunction.hh"
#include "Value.hh"

class Plot_window_properties;
class Plot_data;

/// how long sem_wait_safe() waits for a ⎕PLOT semaphore before giving up.
/// All of the waits it guards (GTK/XCB event loop startup, a plot window
/// being shown, a quick lock/unlock of all_PLOT_windows) normally take
/// well under a second even on a slow (e.g. 32-bit) system; this is
/// generous enough to never fire under legitimate load, while still
/// turning a genuine hang (Bill Heagy, 2026-08-09, /tmp/trouble.eml --
/// on his system the underlying wait was never satisfied at all, for
/// reasons unrelated to speed) into a bounded, reportable failure
/// instead of an indefinite one that only ^C could escape.
enum { PLOT_SEM_TIMEOUT_SECONDS = 15 };

/// outcome of sem_wait_safe(): why the wait ended.
enum PLOT_wait_result
{
   PLOT_WAIT_OK = 0,     ///< the semaphore was actually posted
   PLOT_WAIT_CTRL_C,     ///< the user hit ^C while waiting
   PLOT_WAIT_TIMEOUT,    ///< PLOT_SEM_TIMEOUT_SECONDS elapsed unposted
   PLOT_WAIT_ERROR,      ///< sem_timedwait() itself failed unexpectedly
};

/// sem_wait() that (a) retries on EINTR, except for the user's own ^C,
/// and (b) gives up after PLOT_SEM_TIMEOUT_SECONDS instead of blocking
/// forever if the wait is never satisfied at all. A plain sem_wait()
/// returns prematurely (with the semaphore left un-posted) if a signal
/// arrives while blocked -- including a signal with nothing to do with
/// ⎕PLOT. All of ⎕PLOT's semaphores exist specifically to make the
/// caller wait for some other thread to reach a certain point (e.g.
/// gtk_main() actually running its event loop) before touching GTK from
/// here; a premature, signal-triggered return silently reintroduces
/// exactly the race the wait exists to prevent, since nothing checks
/// sem_wait()'s return value. This is most likely to actually matter on
/// a slow system (e.g. 32-bit), where the real wait is long enough for
/// an unrelated signal to plausibly land before it's satisfied -- so
/// incidental signals are retried transparently. But if the wait is
/// never satisfied at all (a genuine deadlock or lost wakeup in the
/// other thread), blindly retrying forever would make that hang
/// un-interruptible, which is worse than the original race: the user's
/// own ^C (GNU APL installs a real SIGINT handler, see main.cc) must
/// still be able to break out, so EINTR caused by an already-raised
/// attention ends the wait instead of being retried -- and so must a
/// wait that is simply never going to be satisfied.
///
/// This function is called from both the interpreter thread and from
/// the GTK/XCB driver's own callback/event-loop threads, so it must
/// never raise a C++/APL exception itself (Workspace/SI access is not
/// safe from those threads). It returns PLOT_WAIT_OK if the semaphore
/// was actually posted, or a specific PLOT_WAIT_xxx reason otherwise --
/// logged to CERR either way since that is safe from any thread.
/// Callers running on the interpreter thread that want a proper APL
/// error on failure should use sem_wait_safe_I() below instead.
/// @param sema the semaphore to wait for
/// @param what human-readable description of what is being waited for,
///        used only in the CERR diagnostic printed on failure
inline PLOT_wait_result
sem_wait_safe(sem_t * sema, const char * what)
{
   struct timespec deadline;
   clock_gettime(CLOCK_REALTIME, &deadline);
   deadline.tv_sec += PLOT_SEM_TIMEOUT_SECONDS;

   for (;;)
      {
        if (sem_timedwait(sema, &deadline) == 0)   return PLOT_WAIT_OK;

        if (errno == EINTR)
           {
             if (InterruptContext::attention_is_raised())
                {
                  CERR << "*** ⎕PLOT: ^C hit while waiting in "
                          "sem_timedwait() for " << what << endl;
                  return PLOT_WAIT_CTRL_C;
                }
             continue;
           }

        if (errno == ETIMEDOUT)
           {
             CERR << "*** ⎕PLOT: timed out after " << PLOT_SEM_TIMEOUT_SECONDS
                  << "s waiting for " << what << endl;
             return PLOT_WAIT_TIMEOUT;
           }

        CERR << "*** ⎕PLOT: sem_timedwait() while waiting for " << what
             << " failed unexpectedly: " << strerror(errno) << endl;
        return PLOT_WAIT_ERROR;
      }
}

/// like sem_wait_safe() above, but for call sites that run on the
/// interpreter thread (i.e. reached directly from Quad_PLOT::eval_AB()/
/// eval_B(), never from a driver callback thread): raise a DOMAIN_ERROR
/// with a MORE_ERROR() hint instead of silently returning on ^C/timeout.
/// Must NOT be used from a GTK/XCB driver-owned thread -- throwing a
/// C++/APL exception there is not safe, see sem_wait_safe() above.
inline void
sem_wait_safe_I(sem_t * sema, const char * what)
{
const PLOT_wait_result result = sem_wait_safe(sema, what);
   if (result == PLOT_WAIT_OK)   return;

   switch (result)
      {
        case PLOT_WAIT_CTRL_C:
             MORE_ERROR() << "A ⎕PLOT B: ^C was needed to escape "
                             "sem_timedwait() while waiting for " << what
                          << ". Like a timeout, this normally means a "
                             "⎕PLOT driver thread got stuck or died "
                             "unexpectedly -- ^C only ended the wait "
                             "sooner than the " << PLOT_SEM_TIMEOUT_SECONDS
                          << "s timeout would have.";
             break;

        case PLOT_WAIT_TIMEOUT:
             MORE_ERROR() << "A ⎕PLOT B: timed out after "
                          << PLOT_SEM_TIMEOUT_SECONDS << "s while waiting "
                             "for " << what << ". This normally means a "
                             "⎕PLOT driver thread got stuck or died "
                             "unexpectedly.";
             break;

        default:
             MORE_ERROR() << "A ⎕PLOT B: unexpected internal error while "
                             "waiting for " << what << " (see the *** "
                             "⎕PLOT: ... message on stderr for detail).";
             break;
      }
   DOMAIN_ERROR;
}

/// ⎕PLOT verbosity bitmap
enum
{
   SHOW_EVENTS = 1,   ///< show X events
   SHOW_DATA   = 2,   ///< show APL data
   SHOW_DRAW   = 4,   ///< show draw details
};

/// a shrt delay needed when the user wants window borders to be saved
//  into a .png file
enum { SAVE_BORDER_DELAY_ms = 100 };

//════════════════════════════════════════════════════════════════════════════
/** The class implementing ⎕PLOT. \b Quad_PLOT implements the APL
    side of plotting, while the actual output work is performed by
    plot drivers for different GUI environments (X11/XCB or GTK).

    Class \b Quad_PLOT handles the APL aspects of plotting n GNU APL.
    It is accompanied by currently 3 platform dependent driver files
    that do the actual output work:

    1. Plot_ascii.cc (always used and linked in Makefile),
    2. Plot_gtk.cc   (used and linked if apl_GTK3 was set by ./configure), and
    3. Plot_XCB      (used and linked if apl_CCB  was set by ./configure).

    Plot_ascii runs in the same thread as the GNU APL interpreter. It plots
    to stderr and returns a committed value with 3 planes for characters,
    VT100 foreground colors, and VT100 background colors.

    Plot_gtk.cc uses a separate phtread for every plot window. It returns an
    integer window handle which ⎕PLOT case use to manipulate the window (e.g
    to close it from APL).

    Plot_XCB uses a single phtread for all plot windows. It returns an
    integer window handle which ⎕PLOT case use to manipulate the window (e.g
    to close it from APL).

    PLOT synchronizes with the driver thread(s) of GTK or XCB by means of
    two mutuial exclusion semaphores:

    1. all_PLOT_windows_sema: a process semaphore; initially 1, that protects
       the Quad_PLOT::all_PLOT_windows data structure. Used (only by XCB) to
       remove a window from all_PLOT_windows (which could happen simultaneous
       from ⎕PLOT in APL and from an XCB_CLIENT_MESSAGE in XCB. Whoever aquires
       it is allowed change Quad_PLOT::all_PLOT_windows.

    2. expose_sema: a threads semaphore; initially 0, that tells the interpreter
       when a plot window was exposed (shown). Posted by GTK and XCB after the
       plot window was shown. At this point has finishied drawing the initial
       plot window and only reacts to (mouse-) events like window resize,
       window close, etc. It is also the point where Quad_PLOT::start_GUI()
       (and therefore also eval_AB() rersp. eval_B() return. This is to make
       eval_AB() rersp. eval_B() kind of atomic operations.

    A \b default_plot_driver is chosen at compile time based on the
    ./configure result (i.e. \b config.h).

    default_plot_driver = PltDrv_GTK    if GTK AND X11 are avaliable, otherwise
    default_plot_driver = PltDrv_XCB    if XCB is avaiable, otherwise
    PltDrv_ASCII        = PltDrv_ASCII.

   \b eval_B() (which has no plot attributes) always uses \b default_plot_driver.
   \b eval_AB() uses \b default_plot_driver unless a differen driver is
   specified in plot attribute \b w_props->get_gui_driver().
 **/
class Quad_PLOT : public QuadFunction
{
public:
   /// Constructor.
   Quad_PLOT();

   /// a small number that identifies a window
   typedef int Handle;

   /// the GUI-independent part of ⎕PLOT (at APL level).
   class PLOT_context
      {
        public:
           /// constructor
           /// @param h unique window handle assigned to this context
           PLOT_context(Handle h)
           : handle(h)
           {}

           /// return the per-window thread (for GUIs that have one, i.e. XCB).
           virtual pthread_t get_thread() const
              { return 0; }

           /// close the plot window in the GUI
           virtual void plot_stop() = 0;

           /// destructor (shall clean up the GUI-dependent context)
           virtual ~PLOT_context() {}

           /// z unique number that identifies \b this PLOT_context (in
           /// \b all_PLOT_windows).
           const Handle handle;

           /// close \b hnadle from all_PLOT_windows
           /// @param handle window handle to remove from all_PLOT_windows
           static Handle remove_handle(Handle handle);   // GTK only
      };

   /// the GTK window that handles one plot window. Always declared here
   /// (to make doxygen happy, but only implemented if apl_GTK3
   /// @param vp_props pointer to the Plot_window_properties for this window
   /// @param handle window handle assigned to this plot window
   static void plot_main_GTK(void * vp_props, Handle handle);

   /// the pthread that handles all XCB plot windows. Always declared here
   /// (to make doxygen happy, but only implemented if NOT apl_GTK3
   /// @param vp_props pointer to the Plot_window_properties for this window
   static void * plot_main_XCB(void * vp_props);

   /// the native Windows (GDI+) window that handles one plot window.
   /// Always declared here (to make doxygen happy), but only implemented
   /// if MINGW_SRC. Spawns one worker pthread per plot window (mirroring
   /// plot_main_GTK's calling convention) that creates the HWND, draws
   /// into it, and runs its own message loop.
   /// @param vp_props pointer to the Plot_window_properties for this window
   /// @param handle window handle assigned to this plot window
   static void plot_main_WIN32(void * vp_props, Handle handle);

   enum Plot_driver
      {
        PltDrv_GTK   = 1,
        PltDrv_XCB   = 2,
        PltDrv_ASCII = 3,
        PltDrv_WIN32 = 4,

      };

   static int get_verbosity()
      { return verbosity; }

   static Quad_PLOT  fun;          ///< Built-in function.

   /// a semaphore blocking until the plot window has been EXPOSED
   static sem_t * expose_sema;

   /// a semaphore protecting vector all_PLOT_windows
   static sem_t * all_PLOT_windows_sema;

   /// the next handle (-1)
   static Handle next_handle;

   /// all open ⎕PLOT windows.
   static vector<PLOT_context *> all_PLOT_windows;

protected:
   /// Destructor.
   ~Quad_PLOT();

   /// initialize the GUI
   /// @param w_props plot window properties for the new window
   /// @param handle window handle assigned to this plot window
   /// @param driver_type GUI driver to use (GTK, XCB, or ASCII)
   static void start_GUI(Plot_window_properties * w_props, int handle,
                         Plot_driver driver_type);

   /// overloaded Function::eval_AB()
   /// @param A left argument APL value (plot attributes)
   /// @param B right argument APL value (data to plot)
   virtual Token eval_AB(cValue_R A, cValue_R B) const;

   /// overloaded Function::eval_B()
   virtual Token eval_B(cValue_R B) const;

   /// control logging etc. of ⎕PLOT
   Value_P window_control(APL_Integer B) const;

   /// print attribute help text
   static void help();

   /// plot the data (creating a new plot window in X)
   static APL_Integer do_plot_data(Plot_window_properties * w_props,
                                   const Plot_data * data);

   /// close the plot window with \b handle (from APL)
   static Handle plot_stop_APL(Handle handle);

   /// initialize the data to be plotted
   static Plot_data * setup_data(const cValue & B);

   /// initialize the data to be plotted for a 3D plot
   static Plot_data * setup_data_3D(const cValue & B);

   /// initialize the data to be plotted for a 2D plot (except case 2b.)
   static Plot_data * setup_data_2D(const cValue & B);

   /// initialize the data to be plotted for a 2D plot (case 2b.)
   static Plot_data * setup_data_2D_2b(const cValue & B);

   /// parse the (all-optional) attributes in A
   static ErrorCode parse_attributes(const cValue & A,
                                     Plot_window_properties * w_props);

   /// whether to print some debug info during plotting
   static int verbosity;
};
//════════════════════════════════════════════════════════════════════════════

#endif // __Quad_PLOT_DEFINED__
