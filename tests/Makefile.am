check_PROGRAMS = test-library-callback test-library-file
test_library_callback_SOURCES = test-library-callback.c
test_library_callback_CFLAGS = -I $(top_srcdir)/src
test_library_callback_LDADD = $(top_builddir)/src/libofdm-transfer.la
test_library_file_SOURCES = test-library-file.c
test_library_file_CFLAGS = -I $(top_srcdir)/src
test_library_file_LDADD = $(top_builddir)/src/libofdm-transfer.la
TESTS = test-library-callback test-library-file test-program.sh
