#!/bin/sh
#
# Copyright (C) 2010 Nick Schermer <nick@xfce.org>
# Copyright (C) 2021 Hakan Erduman <hakan@erduman.de>
#
# This library is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the Free
# Software Foundation; either version 2 of the License, or (at your option)
# any later version.
#
# This library is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
# more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; If not, see <http://www.gnu.org/licenses/>.
#

export TEXTDOMAIN="xfce4-panel"
export TEXTDOMAINDIR="${CMAKE_INSTALL_FULL_LOCALEDIR}"

ATPOINTER="false"

case "$1" in
  -h|--help)
    echo "$(gettext "Usage:")"
    echo "  $(basename $0) [$(gettext "OPTION")...]"
    echo
    echo "$(gettext "Options:")"
    echo "  -p, --pointer   $(gettext "Popup menu at current mouse position")"
    echo "  -h, --help      $(gettext "Show help options")"
    echo "  -V, --version   $(gettext "Print version information and exit")"
    exit 0
    ;;
  -V|--version)
    exec xfce4-panel -V "$(basename $0)"
    exit 0
    ;;
  -p|--pointer)
    ATPOINTER="true"
    ;;
esac

exec xfce4-panel --plugin-event=hamster:popup:bool:$ATPOINTER

# vim:set ts=2 sw=2 et ai:
