
AM_MAKEFLAGS = -j $(MAKE_J)

AM_CXXFLAGS = $(CXX_RDYNAMIC) -I $(srcdir)/.. -g -O2
if apl_DEVELOP
   AM_CXXFLAGS += $(WERROR) -Wall -Wno-strict-aliasing
endif

pkglib_LTLIBRARIES =	lib_file_io.la		\
			lib_template_F0.la	\
			lib_template_F12.la	\
			lib_template_OP1.la	\
			lib_template_OP2.la	

# set common_LDFLAGS to the LDFLAGS needed by your platform. MacOS seems
# not to like one of the -module, -shared, or -export-dynamic options when
# asked to build dylibs.
#
common_LDFLAGS = -avoid-version
# common_LDFLAGS += -module
# common_LDFLAGS += -shared
common_LDFLAGS += -export-dynamic

lib_file_io_la_SOURCES = file_io.cc
lib_file_io_la_CXXFLAGS = $(AM_CXXFLAGS)
lib_file_io_la_LDFLAGS = $(common_LDFLAGS)

lib_template_F0_la_SOURCES = template_F0.cc template.hh
lib_template_F0_la_CXXFLAGS = $(AM_CXXFLAGS)
lib_template_F0_la_LDFLAGS = $(common_LDFLAGS)

lib_template_F12_la_SOURCES = template_F12.cc template.hh
lib_template_F12_la_CXXFLAGS = $(AM_CXXFLAGS)
lib_template_F12_la_LDFLAGS = $(common_LDFLAGS)

lib_template_OP1_la_SOURCES = template_OP1.cc template.hh
lib_template_OP1_la_CXXFLAGS = $(AM_CXXFLAGS)
lib_template_OP1_la_LDFLAGS = $(common_LDFLAGS)

lib_template_OP2_la_SOURCES = template_OP2.cc template.hh
lib_template_OP2_la_CXXFLAGS = $(AM_CXXFLAGS)
lib_template_OP2_la_LDFLAGS = $(common_LDFLAGS)

# copy the freshly built .so (or, on MacOS, .dylib) files next to the
# apl binary (..) so that the interpreter can be run and tested from
# the build tree without a prior 'make install'.
#
# The trailing ': ;' is needed because an unmatched glob (e.g. *.so on
# MacOS, which builds *.dylib instead) is left as a literal, non-
# existent filename rather than expanding to nothing; the loop's last
# command would then be the failing 'test -f', and since that is the
# exit status Make sees for this recipe line, 'make all' would abort
# right here with no visible error text -- exactly the reported
# 'all-local] Error 1' with an otherwise fully successful build
# (M. Hall, bug-apl@gnu.org, 2026-08-18). This convenience copy should
# never be able to fail the build either way, so force success.
#
all-local: $(pkglib_LTLIBRARIES)
	@for so in .libs/*.so .libs/*.dylib ; do \
	   test -f "$$so" && cp -fp "$$so" .. ; \
	done ; :

