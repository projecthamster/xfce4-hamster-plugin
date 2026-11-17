#!/bin/sh
#  xfce4-hamster-plugin
#
#  Copyright (c) 2026 Hakan Erduman <hakan@erduman.de>
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU Library General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; If not, see <https://www.gnu.org/licenses/>.

# Start a throwaway display, run tests/popup-probe on it, and for Wayland also
# assert that the compositor exports wlr-layer-shell (needed by the panel
# window mode with libxfce4panel >= 4.19). A live xfce4-panel is not started;
# panel anchoring, autohide and click-outside stay untested by design.
#
# usage: popup-runtime.sh x11 PROBE XVFB
#        popup-runtime.sh wayland PROBE SWAY WAYLAND-INFO

set -eu

usage()
{
   echo "usage: $0 x11 PROBE XVFB" >&2
   echo "       $0 wayland PROBE SWAY WAYLAND-INFO" >&2
}

fatal()
{
   echo "$0: $1" >&2
   exit 1
}

dump_log()
{
   if [ -n "${1:-}" ] && [ -s "$1" ]; then
      echo "--- tail of $1 ---" >&2
      tail -n 40 "$1" >&2 || true
      echo "--- end of $1 ---" >&2
   fi
}

if [ "$#" -lt 3 ]; then
   usage
   exit 2
fi

backend=$1
probe=$2

if [ ! -x "$probe" ]; then
   fatal "probe '$probe' is not executable"
fi

tmp=$(mktemp -d "${TMPDIR:-/tmp}/popup-runtime.XXXXXX")
pids=

cleanup()
{
   for pid in $pids; do
      kill "$pid" 2>/dev/null || true
   done
   for pid in $pids; do
      wait "$pid" 2>/dev/null || true
   done
   rm -rf "$tmp"
}
trap cleanup EXIT INT TERM

run_x11()
{
   xvfb=$1
   display_file="$tmp/display"
   xvfb_log="$tmp/xvfb.log"
   : >"$display_file"

   "$xvfb" -displayfd 3 -screen 0 1024x768x24 3>"$display_file" \
      >"$xvfb_log" 2>&1 &
   xvfb_pid=$!
   pids="$pids $xvfb_pid"

   number=
   i=0
   while [ "$i" -lt 100 ]; do
      if [ -s "$display_file" ]; then
         number=$(head -n 1 "$display_file" | tr -d '[:space:]')
         if [ -n "$number" ]; then
            break
         fi
      fi
      if ! kill -0 "$xvfb_pid" 2>/dev/null; then
         break
      fi
      sleep 0.1
      i=$((i + 1))
   done

   if [ -z "$number" ]; then
      dump_log "$xvfb_log"
      fatal "Xvfb did not report a display number within 10 s"
   fi

   echo "popup-runtime: Xvfb display :$number"

   if ! DISPLAY=":$number" GDK_BACKEND=x11 timeout 60 "$probe" --expect x11; then
      dump_log "$xvfb_log"
      fatal "popup-probe failed on X11 (display :$number)"
   fi
}

run_wayland()
{
   sway=$1
   wayland_info=$2
   sway_config="$tmp/sway.conf"
   sway_log="$tmp/sway.log"

   export XDG_RUNTIME_DIR="$tmp/run"
   mkdir -p "$XDG_RUNTIME_DIR"
   chmod 700 "$XDG_RUNTIME_DIR"

   # no Xwayland: the probe must run on the pure Wayland backend, and
   # headless sway serves that fine
   printf '%s\n' 'xwayland disable' >"$sway_config"
   : >"$sway_log"

   WLR_BACKENDS=headless \
   WLR_RENDERER=pixman \
   WLR_LIBINPUT_NO_DEVICES=1 \
      "$sway" -c "$sway_config" >"$sway_log" 2>&1 &
   sway_pid=$!
   pids="$pids $sway_pid"

   socket=
   i=0
   while [ "$i" -lt 150 ]; do
      for candidate in "$XDG_RUNTIME_DIR"/wayland-*; do
         case $candidate in
         *.lock)
            continue
            ;;
         esac
         if [ -S "$candidate" ]; then
            socket=$candidate
            break
         fi
      done
      if [ -n "$socket" ]; then
         break
      fi
      if ! kill -0 "$sway_pid" 2>/dev/null; then
         break
      fi
      sleep 0.1
      i=$((i + 1))
   done

   if [ -z "$socket" ]; then
      dump_log "$sway_log"
      fatal "sway did not create a wayland-* socket within 15 s"
   fi

   WAYLAND_DISPLAY=$(basename "$socket")
   export WAYLAND_DISPLAY
   echo "popup-runtime: sway socket $socket (WAYLAND_DISPLAY=$WAYLAND_DISPLAY)"

   if ! GDK_BACKEND=wayland timeout 60 "$probe" --expect wayland; then
      dump_log "$sway_log"
      fatal "popup-probe failed on Wayland (WAYLAND_DISPLAY=$WAYLAND_DISPLAY)"
   fi

   if ! layer_shell=$("$wayland_info" 2>"$tmp/wayland-info.log" \
         | grep 'zwlr_layer_shell_v1'); then
      echo "$0: the test compositor does not export zwlr_layer_shell_v1;" \
           "popup-wayland cannot cover the panel window path" >&2
      dump_log "$tmp/wayland-info.log"
      dump_log "$sway_log"
      exit 1
   fi
   echo "popup-runtime: layer shell global: $layer_shell"
}

case $backend in
x11)
   if [ "$#" -ne 3 ]; then
      usage
      exit 2
   fi
   run_x11 "$3"
   ;;
wayland)
   if [ "$#" -ne 4 ]; then
      usage
      exit 2
   fi
   run_wayland "$3" "$4"
   ;;
*)
   usage
   exit 2
   ;;
esac
