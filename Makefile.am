AUTOMAKE_OPTIONS = foreign dist-lzip no-dist-gzip subdir-objects
ACLOCAL_AMFLAGS = -I m4

lib_LTLIBRARIES = libofdm-transfer.la
libofdm_transfer_la_SOURCES = src/ofdm-transfer.c
libofdm_transfer_la_LDFLAGS = -version-info 1:0:0

bin_PROGRAMS = ofdm-transfer
ofdm_transfer_SOURCES = src/main.c
ofdm_transfer_LDADD = libofdm-transfer.la

include_HEADERS = src/ofdm-transfer.h

examplesdir =
examples_PROGRAMS = examples/echo-server
examples_echo_server_SOURCES = examples/echo-server.c
examples_echo_server_CFLAGS = -I src
examples_echo_server_LDADD = libofdm-transfer.la

check_PROGRAMS = tests/test-library-callback tests/test-library-file
tests_test_library_callback_SOURCES = tests/test-library-callback.c
tests_test_library_callback_CFLAGS = -I src
tests_test_library_callback_LDADD = libofdm-transfer.la
tests_test_library_file_SOURCES = tests/test-library-file.c
tests_test_library_file_CFLAGS = -I src
tests_test_library_file_LDADD = libofdm-transfer.la
TESTS = tests/test-library-callback tests/test-library-file tests/test-program.sh

EXTRA_DIST = tests/test-program.sh
