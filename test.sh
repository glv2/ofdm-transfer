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

function check_ok()
{
    NAME=$1
    OPTIONS1=$2
    OPTIONS2=$3

    echo "Test: ${NAME}"
    ./ofdm-transfer -t -r io ${OPTIONS1} ${MESSAGE} > ${SAMPLES}
    ./ofdm-transfer -r io ${OPTIONS2} ${DECODED} < ${SAMPLES}
    diff -q ${MESSAGE} ${DECODED} > /dev/null
}

function check_nok()
{
    NAME=$1
    OPTIONS1=$2
    OPTIONS2=$3

    echo "Test: ${NAME}"
    ./ofdm-transfer -t -r io ${OPTIONS1} ${MESSAGE} > ${SAMPLES}
    ./ofdm-transfer -r io ${OPTIONS2} ${DECODED} < ${SAMPLES}
    ! diff -q ${MESSAGE} ${DECODED} > /dev/null
}

check_ok "Default parameters" "" ""
check_ok "Bit rate 1200" "-b 1200" "-b 1200"
check_ok "Bit rate 38400" "-b 38400" "-b 38400"
check_ok "Bit rate 400000" "-b 400000" "-b 400000"
check_nok "Wrong bit rate 9600 19200" "-b 9600" "-b 19200"
check_ok "Frequency offset 200000" "-o 200000" "-o 200000"
check_ok "Frequency offset -123456" "-o -123456" "-o -123456"
check_nok "Wrong frequency offset 200000 250000" "-o 200000" "-o 250000"
check_ok "Sample rate 4000000" "-s 4000000" "-s 4000000"
check_ok "Sample rate 10000000" "-s 10000000" "-s 10000000"
check_nok "Wrong sample rate 1000000 2000000" "-s 1000000" "-s 2000000"
check_ok "FEC Hamming(7/4)" "-e h74" "-e h74"
check_ok "FEC Golay(24/12) and repeat(3)" "-e g2412,rep3" "-e g2412,rep3"
check_ok "Wrong FEC Hamming(7/4) Hamming(12/8)" "-e h74" "-e h128"
check_ok "Id a1B2" "-i a1B2" "-i a1B2"
check_nok "Wrong id ABCD ABC" "-i ABCD" "-i ABC"

rm -f ${MESSAGE} ${DECODED} ${SAMPLES}
echo "All tests passed."
