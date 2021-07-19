#!/bin/sh

# This file is part of ofdm-transfer, a program to send or receive data
# by software defined radio using the OFDM modulation.
#
# Copyright 2021 Guillaume LE VAILLANT
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

set -e

MESSAGE=$(mktemp -t message.XXXXXX)
DECODED=$(mktemp -t decoded.XXXXXX)
SAMPLES=$(mktemp -t samples.XXXXXX)

echo "This is a test transmission using ofdm-transfer." > ${MESSAGE}

function check_ok_io()
{
    NAME=$1
    OPTIONS1=$2
    OPTIONS2=$3

    echo "Test: ${NAME}"
    ./ofdm-transfer -t -r io ${OPTIONS1} ${MESSAGE} > ${SAMPLES}
    ./ofdm-transfer -r io ${OPTIONS2} ${DECODED} < ${SAMPLES}
    diff -q ${MESSAGE} ${DECODED} > /dev/null
}

function check_ok_file()
{
    NAME=$1
    OPTIONS1=$2
    OPTIONS2=$3

    echo "Test: ${NAME}"
    ./ofdm-transfer -t -r file=${SAMPLES} ${OPTIONS1} ${MESSAGE}
    ./ofdm-transfer -r file=${SAMPLES} ${OPTIONS2} ${DECODED}
    diff -q ${MESSAGE} ${DECODED} > /dev/null
}

function check_nok_io()
{
    NAME=$1
    OPTIONS1=$2
    OPTIONS2=$3

    echo "Test: ${NAME}"
    ./ofdm-transfer -t -r io ${OPTIONS1} ${MESSAGE} > ${SAMPLES}
    ./ofdm-transfer -r io ${OPTIONS2} ${DECODED} < ${SAMPLES}
    ! diff -q ${MESSAGE} ${DECODED} > /dev/null
}

function check_nok_file()
{
    NAME=$1
    OPTIONS1=$2
    OPTIONS2=$3

    echo "Test: ${NAME}"
    ./ofdm-transfer -t -r file=${SAMPLES} ${OPTIONS1} ${MESSAGE}
    ./ofdm-transfer -r file=${SAMPLES} ${OPTIONS2} ${DECODED}
    ! diff -q ${MESSAGE} ${DECODED} > /dev/null
}

check_ok_io "Default parameters" "" ""
check_ok_io "Bit rate 1200" "-b 1200" "-b 1200"
check_ok_file "Bit rate 9600" "-b 9600" "-b 9600"
check_ok_io "Bit rate 400000" "-b 400000" "-b 400000"
check_nok_io "Wrong bit rate 9600 19200" "-b 9600" "-b 19200"
check_ok_io "Frequency offset 200000" "-o 200000" "-o 200000"
check_ok_file "Frequency offset -123456" "-o -123456" "-o -123456"
check_nok_io "Wrong frequency offset 200000 250000" "-o 200000" "-o 250000"
check_ok_io "Sample rate 4000000" "-s 4000000" "-s 4000000"
check_ok_file "Sample rate 10000000" "-s 10000000" "-s 10000000"
check_nok_io "Wrong sample rate 1000000 2000000" "-s 1000000" "-s 2000000"
check_ok_io "Modulation apsk16" "-m apsk16" "-m apsk16"
check_nok_file "Wrong modulation apsk16 qpsk" "-m apsk16" "-m qpsk"
check_ok_file "Subcarrier number 100" "-n 100" "-n 100"
check_nok_io "Wrong subcarrier number 64 128" "-n 64" "-n 128"
check_ok_io "FEC Hamming(7/4)" "-e h74" "-e h74"
check_ok_file "FEC Golay(24/12) and repeat(3)" "-e g2412,rep3" "-e g2412,rep3"
check_ok_io "Id a1B2" "-i a1B2" "-i a1B2"
check_nok_file "Wrong id ABCD ABC" "-i ABCD" "-i ABC"

rm -f ${MESSAGE} ${DECODED} ${SAMPLES}
echo "All tests passed."
