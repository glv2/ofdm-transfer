examplesdir =
examples_PROGRAMS = duplex echo-server
duplex_SOURCES = duplex.c
duplex_CFLAGS = -I $(top_srcdir)/src
duplex_LDADD = $(top_builddir)/src/libofdm-transfer.la -lpthread
echo_server_SOURCES = echo-server.c
echo_server_CFLAGS = -I $(top_srcdir)/src
echo_server_LDADD = $(top_builddir)/src/libofdm-transfer.la
