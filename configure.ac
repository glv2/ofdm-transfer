AC_INIT(ofdm-transfer, 1.0.0)
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR(ofdm-transfer.c)

dnl Check for toolchain and install components
AC_PROG_CC
AC_PROG_INSTALL

dnl Check for standard headers
AC_CHECK_HEADERS([complex.h fcntl.h signal.h stdio.h stdlib.h string.h strings.h unistd.h])

dnl Check for functions
AC_CHECK_FUNCS([fcntl])
AC_CHECK_FUNCS([signal])
AC_CHECK_FUNCS([fclose feof fflush fopen fprintf fread fwrite printf])
AC_CHECK_FUNCS([exit free malloc strtof strtol strtoul])
AC_CHECK_FUNCS([bzero memcmp memcpy strcasecmp strchr strcpy strlen])
AC_CHECK_FUNCS([getopt usleep])

dnl Check for libraries
AC_CHECK_HEADERS(math.h, [], AC_MSG_ERROR([math headers required]))
AC_CHECK_LIB(m, ceilf, [], AC_MSG_ERROR([math library required]))

AC_CHECK_HEADERS(liquid/liquid.h, [], AC_MSG_ERROR([liquid-dsp header required]))
AC_CHECK_LIB(liquid, ofdmflexframegen_create, [], AC_MSG_ERROR([liquid-dsp library required]))

AC_CHECK_HEADERS(SoapySDR/Device.h, [], AC_MSG_ERROR([SoapySDR header required]))
AC_CHECK_LIB(SoapySDR, SoapySDRDevice_make, [], AC_MSG_ERROR([SoapySDR library required]))

AC_CONFIG_FILES(Makefile)
AC_OUTPUT
