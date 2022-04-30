#!/bin/sh

# This script makes a PPP connection between two machines using the duplex
# example program.
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

case "$1" in
    server)
        pppd noauth \
             local \
             lock \
             nodefaultroute \
             debug \
             nodetach \
             10.0.0.1:10.0.0.2 \
             pty "./duplex 433800000 434200000"
        ;;

    client)
        pppd noauth \
             local \
             lock \
             nodefaultroute \
             debug \
             nodetach \
             passive \
             pty "./duplex 434200000 433800000"
        ;;

    *)
        echo "" >& 2
        echo "Usage: $0 <server | client>" >& 2
        echo "" >& 2
        exit 1
        ;;
esac
