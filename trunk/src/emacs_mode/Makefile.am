AM_CXXFLAGS = $(CXX_RDYNAMIC) -Wno-deprecated-declarations
AM_LDFLAGS  = -export-dynamic

dist_noinst_DATA =	AUTHORS		\
			LAST_GIT_PULL

pkglib_LTLIBRARIES =	libemacs.la

libemacs_la_SOURCES =						\
		DefCommand.cc		DefCommand.hh		\
		emacs.cc		emacs.hh		\
		FnCommand.cc		FnCommand.hh		\
		FnTagCommand.cc		FnTagCommand.hh		\
		FollowCommand.cc	FollowCommand.hh	\
		GetVarCommand.cc	GetVarCommand.hh	\
		HelpCommand.cc		HelpCommand.hh		\
		Listener.cc		Listener.hh		\
		LockWrapper.cc		LockWrapper.hh		\
		network.cc		network.hh		\
		NetworkCommand.hh				\
		NetworkConnection.cc	NetworkConnection.hh	\
		RunCommand.cc		RunCommand.hh		\
		SicCommand.cc		SicCommand.hh		\
		SiCommand.cc		SiCommand.hh		\
		SendCommand.cc		SendCommand.hh		\
		SystemFnCommand.cc	SystemFnCommand.hh	\
		SystemVariableCommand.cc SystemVariableCommand.hh \
		TcpListener.cc		TcpListener.hh		\
		TempFileWrapper.cc	TempFileWrapper.hh	\
		TraceData.cc		TraceData.hh		\
		UnixSocketListener.cc	UnixSocketListener.hh	\
		util.cc			util.hh			\
		VariablesCommand.cc	VariablesCommand.hh	\
		VersionCommand.cc	VersionCommand.hh

# copy the freshly built .so (or, on MacOS, .dylib) next to the apl
# binary (..) so that the interpreter can be run and tested from the
# build tree without a prior 'make install'.
#
# The trailing ': ;' is needed because an unmatched glob (e.g. *.so on
# MacOS, which builds *.dylib instead) is left as a literal, non-
# existent filename rather than expanding to nothing; the loop's last
# command would then be the failing 'test -f', and since that is the
# exit status Make sees for this recipe line, 'make all' would abort
# right here with no visible error text. This convenience copy should
# never be able to fail the build either way, so force success.
#
all-local: $(pkglib_LTLIBRARIES)
	@for so in .libs/*.so .libs/*.dylib ; do \
	   test -f "$$so" && cp -fp "$$so" .. ; \
	done ; :

.PHONY: git-pull
git-pull:
	rm -Rf tmp
	svn export https://github.com/lokedhs/gnu-apl-mode/trunk/native tmp
	mv -f tmp/*cc tmp/*hh .
	rm -Rf tmp
	@echo "Last GIT pull on `date '+%F %R'`" > LAST_GIT_PULL

