#!/usr/bin/env bash

# This script makes a half-duplex connection between two machines
# using audio, like a sound modem TNC.
#
# Copyright 2022 Guillaume LE VAILLANT
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

SAMPLE_RATE=48000
FREQUENCY=1500
BIT_RATE=1200
MODULATION="bpsk"

# Parameters for an Icom IC-705 radio connected via USB
PCM_IN="alsa_input.usb-Burr-Brown_from_TI_USB_Audio_CODEC-00.analog-stereo"
PCM_OUT="alsa_output.usb-Burr-Brown_from_TI_USB_Audio_CODEC-00.analog-stereo"
AUDIO_GAIN=-35
DEVICE="/dev/ttyACM0"
MODEL=3085
USE_PTT=true

# Parameters for a Yaesu FT-818 radio connected via a Signalink USB interface
# PCM_IN="alsa_input.usb-BurrBrown_from_Texas_Instruments_USB_AUDIO_CODEC-00.analog-stereo"
# PCM_OUT="alsa_output.usb-BurrBrown_from_Texas_Instruments_USB_AUDIO_CODEC-00.analog-stereo"
# AUDIO_GAIN=-6
# USE_PTT=false

# Parameters for a regular sound card
# PCM_IN="alsa_input.pci-0000_17_00.4.analog-stereo"
# PCM_OUT="alsa_output.pci-0000_17_00.4.analog-stereo"
# AUDIO_GAIN=-10
# USE_PTT=false

send()
{
    data="$1"
    if ${USE_PTT}
    then
        rigctl -m ${MODEL} -r ${DEVICE} set_ptt 1
    fi
    echo -n "${data}" | \
        ofdm-transfer -t -a \
                      -r io \
                      -s ${SAMPLE_RATE} \
                      -f ${FREQUENCY} \
                      -m ${MODULATION} \
                      -b ${BIT_RATE} \
                      -g ${AUDIO_GAIN} | \
        sox -q -t s16 -r ${SAMPLE_RATE} -c 1 -L - -t pulseaudio ${PCM_OUT}
    if ${USE_PTT}
    then
        rigctl -m ${MODEL} -r ${DEVICE} set_ptt 0
    fi
}

receive()
{
    sox -q -t pulseaudio ${PCM_IN} -t s16 -r ${SAMPLE_RATE} -c 1 -L - | \
        ofdm-transfer -a -r io \
                      -s ${SAMPLE_RATE} \
                      -f ${FREQUENCY} \
                      -m ${MODULATION} \
                      -b ${BIT_RATE} \
                      -g ${AUDIO_GAIN} &
    sox_pid=$(pidof "sox")
    ofdm_transfer_pid=$(pidof "ofdm-transfer")
}

main_loop()
{
    echo "Ctrl-D to quit" 1>&2
    echo 1>&2
    receive
    IFS=""
    while true
    do
        stop=false
        read -r -d "" -t 1 data
        if [ $? -eq 1 -a -z "${data}" ]
        then
            break
        fi
        if [ ${#data} -eq 1 ]
        then
            ret=$(printf "%d" "'${data}")
            if [ ${ret} -eq 4 ]
            then
                break
            else
                send "${data}"
            fi
        elif [ -n "${data}" ]
        then
            send "${data}"
        fi
    done
    kill ${sox_pid} ${ofdm_transfer_pid}
}

main_loop
