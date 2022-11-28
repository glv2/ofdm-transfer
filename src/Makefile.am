lib_LTLIBRARIES = libofdm-transfer.la
libofdm_transfer_la_SOURCES = gettext.h ofdm-transfer.c ofdm-transfer.h
libofdm_transfer_la_LDFLAGS = -version-info 1:0:0

bin_PROGRAMS = ofdm-transfer
ofdm_transfer_SOURCES = gettext.h main.c ofdm-transfer.h
ofdm_transfer_CFLAGS = -DLOCALEDIR=\"$(localedir)\"
ofdm_transfer_LDADD = libofdm-transfer.la
