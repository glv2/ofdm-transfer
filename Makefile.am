AUTOMAKE_OPTIONS = foreign dist-lzip no-dist-gzip subdir-objects
ACLOCAL_AMFLAGS = -I m4

lib_LTLIBRARIES = libofdm-transfer.la
libofdm_transfer_la_SOURCES = src/ofdm-transfer.c
libofdm_transfer_la_LDFLAGS = -version-info 1:0:0

bin_PROGRAMS = ofdm-transfer
ofdm_transfer_SOURCES = src/main.c
ofdm_transfer_LDADD = libofdm-transfer.la

include_HEADERS = src/ofdm-transfer.h

check_PROGRAMS = tests/test-library
tests_test_library_SOURCES = tests/test-library.c
tests_test_library_CFLAGS = -I src
tests_test_library_LDADD = libofdm-transfer.la
TESTS = tests/test-library tests/test-program.sh

EXTRA_DIST = tests/test-program.sh
