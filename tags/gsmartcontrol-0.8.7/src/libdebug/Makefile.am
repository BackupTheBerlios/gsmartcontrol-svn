AM_CPPFLAGS = $(all_includes)
METASOURCES = AUTO
noinst_HEADERS = dchannel.h dcmdarg.h dexcept.h dflags.h dout.h dstate.h \
	dstate_pub.h dstream.h libdebug.h libdebug_mini.h
noinst_LIBRARIES = libdebug.a
libdebug_a_SOURCES = dchannel.cpp dcmdarg.cpp dflags.cpp dout.cpp dstate.cpp \
	dstream.cpp

# don't use absolute path for the current dir's .a, because the make
# dependency resolver won't get it (needed for parallel builds)

libdebug_test_SOURCES = libdebug_test.cpp
libdebug_test_LDADD = libdebug.a


# we don't list them in a separate variable because otherwise kdevelop won't see them.
noinst_PROGRAMS =
if ENABLE_TESTS
noinst_PROGRAMS += libdebug_test
endif

tests: $(noinst_PROGRAMS)


