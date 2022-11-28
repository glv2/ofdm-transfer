AC_INIT(ofdm-transfer, 1.7.2)
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR(src/ofdm-transfer.c)
AC_CONFIG_MACRO_DIRS([m4])

dnl Check for toolchain and install components
AC_PROG_CC
AC_PROG_INSTALL
LT_INIT([shared disable-static])

dnl Check for translation tools
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.19.1])
AM_GNU_GETTEXT_REQUIRE_VERSION([0.19.1])

dnl Check for standard headers
AC_CHECK_HEADERS([complex.h fcntl.h locale.h signal.h stdio.h stdlib.h string.h strings.h unistd.h])

dnl Check for functions
AC_CHECK_FUNCS([fcntl])
AC_CHECK_FUNCS([bindtextdomain setlocale textdomain])
AC_CHECK_FUNCS([signal])
AC_CHECK_FUNCS([fclose feof fflush fopen fprintf fread fwrite printf])
AC_CHECK_FUNCS([exit free malloc strtof strtol strtoul])
AC_CHECK_FUNCS([bzero memcmp memcpy strcasecmp strchr strcpy strlen strncasecmp])
AC_CHECK_FUNCS([getopt usleep])

dnl Check for libraries
AC_CHECK_HEADERS(math.h, [], AC_MSG_ERROR([math headers required]))
AC_CHECK_LIB(m, ceilf, [], AC_MSG_ERROR([math library required]))

AC_CHECK_HEADERS(liquid/liquid.h, [], AC_MSG_ERROR([liquid-dsp header required]))
AC_CHECK_LIB(liquid, ofdmflexframegen_create, [], AC_MSG_ERROR([liquid-dsp library required]))

AC_CHECK_HEADERS(SoapySDR/Device.h, [], AC_MSG_ERROR([SoapySDR header required]))
AC_CHECK_LIB(SoapySDR, SoapySDRDevice_make, [], AC_MSG_ERROR([SoapySDR library required]))

AC_CHECK_HEADERS(pthread.h, [], AC_MSG_ERROR([pthread headers required]))
AC_CHECK_LIB(pthread, pthread_create, [], AC_MSG_ERROR([pthread library required]))

AC_CONFIG_FILES(Makefile examples/Makefile po/Makefile.in src/Makefile tests/Makefile)
AC_OUTPUT
