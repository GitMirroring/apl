
gnuapldir = $(sysconfdir)/gnu-apl.d
dist_noinst_DATA = preferences.in
dist_gnuapl_DATA = keyboard1.txt parallel_thresholds
nodist_gnuapl_DATA = preferences

# preferences is made here rather than by configure, so that the
# installation directories in it are fully expanded
preferences: preferences.in Makefile
	$(AM_V_GEN)$(SED) -e 's|[@]pkglibdir[@]|$(pkglibdir)|g'   \
	                  -e 's|[@]sysconfdir[@]|$(sysconfdir)|g' \
	                  $(srcdir)/preferences.in > $@

CLEANFILES = preferences
