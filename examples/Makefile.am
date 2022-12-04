examplesdir =
examples_PROGRAMS = full-duplex echo-server
full_duplex_SOURCES = full-duplex.c
full_duplex_CFLAGS = -I $(top_srcdir)/src
full_duplex_LDADD = $(top_builddir)/src/libofdm-transfer.la -lpthread
echo_server_SOURCES = echo-server.c
echo_server_CFLAGS = -I $(top_srcdir)/src
echo_server_LDADD = $(top_builddir)/src/libofdm-transfer.la
