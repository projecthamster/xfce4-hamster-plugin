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

# Live xfce4-panel popup test.
#
# Starts a real xfce4-panel with the built plugin from a throwaway prefix, a
# private HOME and a private session bus, then drives the popup through the
# real plugin event path (xfce4-popup-hamstermenu -> xfce4-panel
# --plugin-event) and through clicks on the plugin button.
#
# X11 (Xvfb + xfwm4): window mode maps a utility toplevel anchored below the
# button, -p maps it at the pointer, a real focus change closes it, the button
# toggles it, and a popup-mode change while it is open is deferred. Popover and
# auto modes create a GtkPopover, which is not visibly rendered under the test
# servers (even for a plain GTK3 application), so only the routing (no mapped
# toplevel) is asserted; popover visibility stays a manual desktop check.
#
# Wayland (headless sway): auto/popover must not create a layer surface, which
# proves the routing. Window mode is asserted end to end: a new "new layer
# surface" line in the sway -d log plus popup pixels in a grim screenshot,
# anchored below the panel. GtkPopover is not visibly rendered under the
# headless test servers, so popover pixels are never asserted. Click-outside
# dismissal is not asserted on Wayland: there is no global pointer grab.
#
# usage: popup-panel.sh x11 LIBHAMPSTER DESKTOP PREFIX LIBDIR DATAROOT POPUP_SCRIPT XVFB XFWM4 XDOTOOL XWININFO XPROP XMESSAGE DBUS_DAEMON XFCONF_QUERY
#        popup-panel.sh wayland LIBHAMPSTER DESKTOP PREFIX LIBDIR DATAROOT POPUP_SCRIPT SWAY SWAYMSG GRIM INPUT IMAGE_DIFF DBUS_DAEMON XFCONF_QUERY

set -eu

# Stable parsing of xwininfo/xprop output and of the sway log across locales.
export LC_ALL=C

# The test's own panel layout, not a panel API: write_panel_config below puts
# the single plugin at the top-left of a 30px panel, so the reserved strip is
# 31 px including the frame/border and the button extends to x=72.
WAYLAND_PANEL_STRIP=31
WAYLAND_BUTTON_RIGHT=72

usage()
{
   echo "usage: $0 x11 LIBHAMPSTER DESKTOP PREFIX LIBDIR DATAROOT POPUP_SCRIPT XVFB XFWM4 XDOTOOL XWININFO XPROP XMESSAGE DBUS_DAEMON XFCONF_QUERY" >&2
   echo "       $0 wayland LIBHAMPSTER DESKTOP PREFIX LIBDIR DATAROOT POPUP_SCRIPT SWAY SWAYMSG GRIM INPUT IMAGE_DIFF DBUS_DAEMON XFCONF_QUERY" >&2
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

# Fail with the message and the logs a reader needs to see why. Every started
# process writes into one of these files, so a failing assertion leaves a
# usable trail instead of only a line number.
fail()
{
   echo "$0: FAIL: $1" >&2
   dump_log "${panel_log:-}"
   dump_log "${wm_log:-}"
   dump_log "${sway_log:-}"
   dump_log "${xvfb_log:-}"
   exit 1
}

require_file()
{
   [ -f "$2" ] || fatal "$1 '$2' is not a regular file"
}

require_exec()
{
   [ -x "$2" ] || fatal "$1 '$2' is not executable"
}

if [ "$#" -lt 2 ]; then
   usage
   exit 2
fi

test_mode=$1

case $test_mode in
x11)
   if [ "$#" -ne 15 ]; then
      usage
      exit 2
   fi
   LIBHAMPSTER=$2
   DESKTOP=$3
   PREFIX=$4
   LIBDIR=$5
   DATAROOT=$6
   POPUP_SCRIPT=$7
   XVFB=$8
   XFWM4=$9
   shift 9
   XDOTOOL=$1
   XWININFO=$2
   XPROP=$3
   XMESSAGE=$4
   DBUS_DAEMON=$5
   XFCONF_QUERY=$6
   ;;
wayland)
   if [ "$#" -ne 14 ]; then
      usage
      exit 2
   fi
   LIBHAMPSTER=$2
   DESKTOP=$3
   PREFIX=$4
   LIBDIR=$5
   DATAROOT=$6
   POPUP_SCRIPT=$7
   SWAY=$8
   SWAYMSG=$9
   shift 9
   GRIM=$1
   INPUT=$2
   IMAGE_DIFF=$3
   DBUS_DAEMON=$4
   XFCONF_QUERY=$5
   ;;
*)
   usage
   exit 2
   ;;
esac

require_file "plugin library" "$LIBHAMPSTER"
require_file "plugin desktop file" "$DESKTOP"
require_file "popup script" "$POPUP_SCRIPT"
require_exec "dbus-daemon" "$DBUS_DAEMON"
require_exec "xfconf-query" "$XFCONF_QUERY"

if [ "$test_mode" = x11 ]; then
   require_exec "Xvfb" "$XVFB"
   require_exec "xfwm4" "$XFWM4"
   require_exec "xdotool" "$XDOTOOL"
   require_exec "xwininfo" "$XWININFO"
   require_exec "xprop" "$XPROP"
   require_exec "xmessage" "$XMESSAGE"
else
   require_exec "sway" "$SWAY"
   require_exec "swaymsg" "$SWAYMSG"
   require_exec "grim" "$GRIM"
   require_exec "popup-wayland-input" "$INPUT"
   require_exec "popup-image-diff" "$IMAGE_DIFF"
fi

tmp=$(mktemp -d "${TMPDIR:-/tmp}/popup-panel.XXXXXX")
pids=
panel_log=
wm_log=
xvfb_log=
sway_log=
panel_pid=
sway_pid=
wm_pid=
helper_pid=
helper_id=
helper_geom=
panel_info=
button_info=
popup_info=
panel_id=
panel_geom=
button_id=
button_geom=
panel_x=0
panel_y=0
panel_w=0
panel_h=0
panel_bottom=0
button_x=0
button_y=0
button_w=0
button_h=0
panel_strip=0

cleanup()
{
   # pids is newest first, so the panel dies before its bus and display
   for pid in $pids; do
      kill "$pid" 2>/dev/null || true
   done
   for pid in $pids; do
      wait "$pid" 2>/dev/null || true
   done
   rm -rf "$tmp"
}
trap cleanup EXIT INT TERM

# The panel only looks for plugins beside its own data dir, so lay out the
# throwaway prefix exactly like an installation: libhamster.so under the
# configured libdir and hamster.desktop under the dataroot, with the prefix
# prepended to XDG_DATA_DIRS.
prepare_environment()
{
   root=$tmp/root
   home=$tmp/home
   config_home=$tmp/xdg/config
   data_home=$tmp/xdg/data
   cache_home=$tmp/xdg/cache
   runtime_dir=$tmp/run

   mkdir -p "$home" "$config_home/xfce4/xfconf/xfce-perchannel-xml" \
            "$data_home" "$cache_home" "$runtime_dir"
   chmod 700 "$runtime_dir"

   HOME=$home
   XDG_CONFIG_HOME=$config_home
   XDG_DATA_HOME=$data_home
   XDG_CACHE_HOME=$cache_home
   XDG_RUNTIME_DIR=$runtime_dir
   export HOME XDG_CONFIG_HOME XDG_DATA_HOME XDG_CACHE_HOME XDG_RUNTIME_DIR
   export XDG_DATA_DIRS="$root$PREFIX/$DATAROOT:/usr/local/share:/usr/share"

   mkdir -p "$root$PREFIX/$LIBDIR/xfce4/panel/plugins" \
            "$root$PREFIX/$DATAROOT/xfce4/panel/plugins"
   cp "$LIBHAMPSTER" "$root$PREFIX/$LIBDIR/xfce4/panel/plugins/libhamster.so"
   cp "$DESKTOP" "$root$PREFIX/$DATAROOT/xfce4/panel/plugins/hamster.desktop"

   ensure_passwd_entry
}

write_panel_config()
{
   popup_mode=$1
   cat >"$XDG_CONFIG_HOME/xfce4/xfconf/xfce-perchannel-xml/xfce4-panel.xml" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<channel name="xfce4-panel" version="1.0">
 <property name="configver" type="int" value="2"/>
 <property name="panels" type="array">
  <value type="int" value="1"/>
  <property name="panel-1" type="empty">
   <property name="position" type="string" value="p=6;x=0;y=0"/>
   <property name="length" type="double" value="100"/>
   <property name="position-locked" type="bool" value="true"/>
   <property name="size" type="uint" value="30"/>
   <property name="plugin-ids" type="array">
    <value type="int" value="1"/>
   </property>
  </property>
 </property>
 <property name="plugins" type="empty">
  <property name="plugin-1" type="string" value="hamster">
   <property name="popup-mode" type="string" value="$popup_mode"/>
  </property>
 </property>
</channel>
EOF
}

# Without a window manager the panel is not positioned and the focus-change
# dismissal cannot happen; these three settings keep xfwm4 out of the way.
write_xfwm4_config()
{
   cat >"$XDG_CONFIG_HOME/xfce4/xfconf/xfce-perchannel-xml/xfwm4.xml" <<'EOF'
<?xml version="1.0" encoding="UTF-8"?>
<channel name="xfwm4" version="1.0">
 <property name="general" type="empty">
  <property name="focus_follows_mouse" type="bool" value="false"/>
  <property name="click_to_focus" type="bool" value="true"/>
  <property name="use_compositing" type="bool" value="false"/>
 </property>
</channel>
EOF
}

# dbus-daemon resolves the current uid through the password database before it
# will start a session bus. make docker-test runs as an arbitrary host uid that
# the image does not know, so supply a synthetic passwd and group entry through
# libnss-wrapper when the uid does not resolve; a local run, whose user exists,
# needs nothing. $home must exist already.
ensure_passwd_entry()
{
   uid=$(id -u)
   gid=$(id -g)

   if getent passwd "$uid" >/dev/null 2>&1; then
      return 0
   fi

   wrapper=
   for candidate in /usr/lib/*/libnss_wrapper.so /lib/*/libnss_wrapper.so \
         /usr/lib/libnss_wrapper.so; do
      if [ -f "$candidate" ]; then
         wrapper=$candidate
         break
      fi
   done
   if [ -z "$wrapper" ]; then
      fatal "uid $uid has no passwd entry and libnss-wrapper is not installed; the session bus needs a passwd entry for the current uid, and libnss-wrapper provides one"
   fi

   printf 'root:x:0:0:root:/root:/bin/sh\n' >"$tmp/passwd"
   printf 'hamster-test:x:%s:%s:hamster test:%s:/bin/sh\n' \
      "$uid" "$gid" "$home" >>"$tmp/passwd"
   printf 'root:x:0:\n' >"$tmp/group"
   printf 'hamster-test:x:%s:\n' "$gid" >>"$tmp/group"

   NSS_WRAPPER_PASSWD=$tmp/passwd
   NSS_WRAPPER_GROUP=$tmp/group
   LD_PRELOAD=${LD_PRELOAD:+$LD_PRELOAD:}$wrapper
   export NSS_WRAPPER_PASSWD NSS_WRAPPER_GROUP LD_PRELOAD
   echo "ok synthetic passwd entry for uid $uid (libnss_wrapper)"
}

# xfconf needs a session bus, and ensure_passwd_entry makes the uid resolvable
# for dbus-daemon when the image does not know it.
start_bus()
{
   dbus_log=$tmp/dbus.log
   bus_lines=$("$DBUS_DAEMON" --session --fork --print-address=1 \
         --print-pid=1 2>"$dbus_log") \
      || {
         dump_log "$dbus_log"
         fail "dbus-daemon could not start a private session bus"
      }
   bus_address=$(printf '%s\n' "$bus_lines" | sed -n '1p')
   bus_pid=$(printf '%s\n' "$bus_lines" | sed -n '2p')
   if [ -z "$bus_address" ] || [ -z "$bus_pid" ]; then
      dump_log "$dbus_log"
      fail "dbus-daemon did not print an address and pid"
   fi
   DBUS_SESSION_BUS_ADDRESS=$bus_address
   export DBUS_SESSION_BUS_ADDRESS
   pids="$bus_pid $pids"
   echo "ok private session bus on $bus_address"
}

xfconf_set_mode()
{
   "$XFCONF_QUERY" -c xfce4-panel -p /plugins/plugin-1/popup-mode -s "$1" \
      >/dev/null 2>&1 \
      || fail "xfconf-query could not set popup-mode to '$1'"
}

start_xvfb()
{
   display_file=$tmp/display
   xvfb_log=$tmp/xvfb.log
   : >"$display_file"

   "$XVFB" -displayfd 3 -screen 0 1280x720x24 3>"$display_file" \
      >"$xvfb_log" 2>&1 &
   xvfb_pid=$!
   pids="$xvfb_pid $pids"

   display_number=
   i=0
   while [ "$i" -lt 100 ]; do
      if [ -s "$display_file" ]; then
         display_number=$(head -n 1 "$display_file" | tr -d '[:space:]')
         if [ -n "$display_number" ]; then
            break
         fi
      fi
      if ! kill -0 "$xvfb_pid" 2>/dev/null; then
         break
      fi
      sleep 0.1
      i=$((i + 1))
   done

   if [ -z "$display_number" ]; then
      dump_log "$xvfb_log"
      fail "Xvfb did not report a display number within 10 s"
   fi

   DISPLAY=":$display_number"
   export DISPLAY
   echo "ok Xvfb on :$display_number"
}

wait_wm()
{
   i=0
   while [ "$i" -lt 100 ]; do
      if "$XPROP" -root _NET_SUPPORTING_WM_CHECK 2>/dev/null \
            | grep -q 'window id # 0x'; then
         return 0
      fi
      if ! kill -0 "$wm_pid" 2>/dev/null; then
         return 1
      fi
      sleep 0.1
      i=$((i + 1))
   done
   return 1
}

start_wm()
{
   wm_log=$tmp/xfwm4.log
   "$XFWM4" --replace >"$wm_log" 2>&1 &
   wm_pid=$!
   pids="$wm_pid $pids"
   if ! wait_wm; then
      fail "xfwm4 did not become the window manager within 10 s"
   fi
   echo "ok xfwm4 is running"
}

start_panel()
{
   panel_log=$tmp/panel.log
   xfce4-panel --disable-wm-check >"$panel_log" 2>&1 &
   panel_pid=$!
   pids="$panel_pid $pids"
}

# Print "X Y W H" for a viewable window, or nothing when it is gone.
win_geometry()
{
   "$XWININFO" -id "$1" 2>/dev/null | awk '
      /Map State: IsViewable/ { viewable = 1 }
      /Absolute upper-left X:/ { x = $NF }
      /Absolute upper-left Y:/ { y = $NF }
      /Width:/ { w = $NF }
      /Height:/ { h = $NF }
      END {
         if (viewable && w != "" && h != "")
            print x, y, w, h
      }'
}

split_geometry()
{
   GX=${1%% *}
   rest=${1#* }
   GY=${rest%% *}
   rest=${rest#* }
   GW=${rest%% *}
   GH=${rest#* }
}

# The panel is a large dock-sized toplevel. Prefer its WM_CLASS and fall back
# to the WM client list with a DOCK hint, because the class is not part of any
# stable panel API.
find_panel_window()
{
   for id in $("$XDOTOOL" search --onlyvisible --class xfce4-panel \
         2>/dev/null || true); do
      geom=$(win_geometry "$id")
      [ -n "$geom" ] || continue
      split_geometry "$geom"
      if [ "$GW" -ge 200 ] && [ "$GH" -ge 20 ]; then
         echo "$id $geom"
         return 0
      fi
   done

   for id in $("$XPROP" -root _NET_CLIENT_LIST 2>/dev/null \
         | sed -n 's/.*# //p' | tr ',' ' '); do
      types=$("$XPROP" -id "$id" _NET_WM_WINDOW_TYPE 2>/dev/null || true)
      case $types in
      *_NET_WM_WINDOW_TYPE_DOCK*) ;;
      *) continue ;;
      esac
      geom=$(win_geometry "$id")
      [ -n "$geom" ] || continue
      split_geometry "$geom"
      if [ "$GW" -ge 200 ] && [ "$GH" -ge 20 ]; then
         echo "$id $geom"
         return 0
      fi
   done
   return 1
}

# The plugin button shares WM_CLASS wrapper-2.0 with the popup toplevel; the
# button is the small one.
find_button()
{
   for id in $("$XDOTOOL" search --onlyvisible --class wrapper-2.0 \
         2>/dev/null || true); do
      geom=$(win_geometry "$id")
      [ -n "$geom" ] || continue
      split_geometry "$geom"
      if [ "$GW" -ge 40 ] && [ "$GH" -ge 20 ]; then
         echo "$id $geom"
         return 0
      fi
   done
   return 1
}

# The window-mode popup is a nested utility toplevel with the wrapper class,
# larger than the button and viewable.
find_popup()
{
   skip_id=$1
   for id in $("$XDOTOOL" search --onlyvisible --class wrapper-2.0 \
         2>/dev/null || true); do
      [ "$id" = "$skip_id" ] && continue
      types=$("$XPROP" -id "$id" _NET_WM_WINDOW_TYPE 2>/dev/null || true)
      case $types in
      *_NET_WM_WINDOW_TYPE_UTILITY*) ;;
      *) continue ;;
      esac
      geom=$(win_geometry "$id")
      [ -n "$geom" ] || continue
      echo "$id $geom"
      return 0
   done
   return 1
}

wait_panel_x11()
{
   i=0
   while [ "$i" -lt 150 ]; do
      if panel_info=$(find_panel_window) \
            && button_info=$(find_button); then
         return 0
      fi
      if ! kill -0 "$panel_pid" 2>/dev/null; then
         return 1
      fi
      sleep 0.1
      i=$((i + 1))
   done
   return 1
}

wait_popup()
{
   i=0
   while [ "$i" -lt 50 ]; do
      if popup_info=$(find_popup "$button_id"); then
         return 0
      fi
      sleep 0.1
      i=$((i + 1))
   done
   return 1
}

wait_popup_gone()
{
   limit=$(($1 * 10))
   i=0
   while [ "$i" -lt "$limit" ]; do
      if ! find_popup "$button_id" >/dev/null 2>&1; then
         return 0
      fi
      sleep 0.1
      i=$((i + 1))
   done
   return 1
}

click_at()
{
   "$XDOTOOL" mousemove "$1" "$2" >/dev/null 2>&1 || return 1
   "$XDOTOOL" click 1 >/dev/null 2>&1 || return 1
}

click_button()
{
   click_at "$((button_x + button_w / 2))" "$((button_y + button_h / 2))"
}

click_helper()
{
   split_geometry "$helper_geom"
   click_at "$((GX + GW / 2))" "$((GY + GH / 2))"
}

start_helper()
{
   "$XMESSAGE" -geometry 200x100+300+300 helper >"$tmp/xmessage.log" 2>&1 &
   helper_pid=$!
   pids="$helper_pid $pids"

   i=0
   while [ "$i" -lt 100 ]; do
      for id in $("$XDOTOOL" search --onlyvisible --class xmessage \
            2>/dev/null || true); do
         geom=$(win_geometry "$id")
         [ -n "$geom" ] || continue
         helper_id=$id
         helper_geom=$geom
         return 0
      done
      sleep 0.1
      i=$((i + 1))
   done
   return 1
}

check_at_pointer()
{
   popup_geom=${popup_info#* }
   split_geometry "$popup_geom"
   dx=$((GX - 640))
   if [ "$dx" -lt 0 ]; then
      dx=$((-dx))
   fi
   dy=$((GY - 400))
   if [ "$dy" -lt 0 ]; then
      dy=$((-dy))
   fi
   if [ "$dx" -gt 10 ] || [ "$dy" -gt 10 ]; then
      fail "at-pointer popup is at $GX,$GY, not within 10 px of 640,400"
   fi
   echo "ok at-pointer popup mapped at $GX,$GY"
}

check_anchored_popup()
{
   label=$1
   popup_geom=${popup_info#* }
   split_geometry "$popup_geom"
   popup_x=$GX
   popup_y=$GY
   popup_w=$GW
   popup_h=$GH

   if [ "$popup_y" -lt "$((panel_bottom - 1))" ] \
         || [ "$popup_y" -gt "$((panel_bottom + 40))" ]; then
      fail "$label: popup y=$popup_y is not just below the panel bottom y=$panel_bottom"
   fi
   if [ "$popup_w" -lt 120 ] || [ "$popup_h" -lt 100 ]; then
      fail "$label: popup ${popup_w}x${popup_h} is smaller than 120x100"
   fi
   if [ "$popup_x" -ge "$((button_x + button_w))" ] \
         || [ "$((popup_x + popup_w))" -le "$button_x" ]; then
      fail "$label: popup x=$popup_x..$((popup_x + popup_w)) does not overlap the button x=$button_x..$((button_x + button_w))"
   fi
   echo "ok $label: ${popup_w}x${popup_h} popup at $popup_x,$popup_y, anchored below the panel"
}

# Popover and auto modes do not produce a toplevel on X11, and GtkPopover is
# not visibly rendered under the test servers (even for a plain GTK3
# application), so this asserts the routing only; whether a popover is visible
# on a real desktop is a manual check.
assert_no_utility()
{
   label=$1
   sleep 0.6
   if found_popup=$(find_popup "$button_id"); then
      fail "$label: a mapped UTILITY toplevel appeared although the popover path should have been selected"
   fi
   if ! kill -0 "$panel_pid" 2>/dev/null; then
      fail "$label: the panel died"
   fi
   echo "ok $label: no mapped UTILITY toplevel, popover routing selected (visibility not asserted)"
}

run_x11()
{
   prepare_environment
   write_panel_config window
   write_xfwm4_config
   export GDK_BACKEND=x11

   start_bus
   start_xvfb
   start_wm
   start_panel

   if ! wait_panel_x11; then
      fail "the panel and its plugin button did not appear within 15 s"
   fi

   panel_id=${panel_info%% *}
   panel_geom=${panel_info#* }
   split_geometry "$panel_geom"
   panel_x=$GX
   panel_y=$GY
   panel_w=$GW
   panel_h=$GH
   panel_bottom=$((panel_y + panel_h))

   button_id=${button_info%% *}
   button_geom=${button_info#* }
   split_geometry "$button_geom"
   button_x=$GX
   button_y=$GY
   button_w=$GW
   button_h=$GH

   if [ "$button_x" -lt "$((panel_x - 1))" ] \
         || [ "$button_y" -lt "$((panel_y - 1))" ] \
         || [ "$((button_x + button_w))" -gt "$((panel_x + panel_w + 1))" ] \
         || [ "$((button_y + button_h))" -gt "$((panel_y + panel_h + 1))" ]; then
      fail "plugin button at $button_x,$button_y ${button_w}x${button_h} is not inside the panel at $panel_x,$panel_y ${panel_w}x${panel_h}"
   fi
   echo "ok panel ${panel_w}x${panel_h} at $panel_x,$panel_y with a ${button_w}x${button_h} plugin button"

   if ! start_helper; then
      fail "the xmessage helper window did not appear"
   fi

   # 1. the at-pointer path of the real popup script
   "$XDOTOOL" mousemove 640 400 >/dev/null 2>&1 \
      || fail "xdotool could not move the pointer"
   sh "$POPUP_SCRIPT" -p || fail "the popup script failed with -p"
   wait_popup || fail "no UTILITY popup appeared after the at-pointer event"
   check_at_pointer

   # 2. a real focus change dismisses it; clicking the panel itself does not
   click_helper || fail "could not click the helper window"
   wait_popup_gone 3 \
      || fail "the popup did not close within 3 s after a focus change"
   # Let the plugin finish its hide before the next open; otherwise the popup
   # event can still see the old "shown" state and toggle it off.
   sleep 0.2
   echo "ok focus change via the helper window closed the at-pointer popup"

   # 3. window mode anchored below the button
   sh "$POPUP_SCRIPT" || fail "the popup script failed"
   wait_popup || fail "no UTILITY popup appeared after the anchored event"
   check_anchored_popup "anchored window popup"

   # 4. changing the mode while the window is open is deferred
   xfconf_set_mode popover
   sleep 0.5
   if ! find_popup "$button_id" >/dev/null 2>&1; then
      fail "the open window popup closed when popup-mode changed to popover (expected deferred)"
   fi
   echo "ok popup-mode change to popover is deferred while the window popup is open"

   click_helper || fail "could not click the helper window"
   wait_popup_gone 3 \
      || fail "the deferred window popup did not close after a focus change"
   sleep 0.2

   # 5. the popover path: no toplevel, and the plugin survives it
   click_button || fail "could not click the plugin button"
   assert_no_utility "popover mode"
   click_button || fail "could not click the plugin button to close the popover"
   sleep 0.3

   # 6. back to window mode: the next open is a real toplevel again
   xfconf_set_mode window
   sleep 0.3
   click_button || fail "could not click the plugin button"
   wait_popup || fail "no UTILITY popup after switching back to window mode"
   check_anchored_popup "window popup after the deferred switch"
   echo "ok window mode opened a mapped toplevel again"

   # Close it before switching to auto: an open window survives a mode change,
   # and the auto assertion below would then only observe that open window.
   click_button || fail "could not click the plugin button"
   wait_popup_gone 3 || fail "the window popup did not close"
   sleep 0.2

   # 7. auto mode selects the popover path too
   xfconf_set_mode auto
   sleep 0.3
   click_button || fail "could not click the plugin button"
   assert_no_utility "auto mode"
   click_button || fail "could not click the plugin button to close the popover"
   sleep 0.3

   # 8. the panel and plugin must still be there
   if ! wait_panel_x11; then
      fail "the panel or the plugin button disappeared during the test"
   fi
   echo "ok panel and plugin are still alive"
   echo "popup-panel: all X11 assertions passed"
}

start_sway()
{
   sway_config=$tmp/sway.conf
   sway_log=$tmp/sway.log

   # no Xwayland: the panel must use its pure Wayland path
   printf '%s\n' 'xwayland disable' >"$sway_config"
   : >"$sway_log"

   WLR_BACKENDS=headless \
   WLR_RENDERER=pixman \
   WLR_LIBINPUT_NO_DEVICES=1 \
      "$SWAY" -c "$sway_config" -d >"$sway_log" 2>&1 &
   sway_pid=$!
   pids="$sway_pid $pids"

   wayland_socket=
   sway_socket=
   i=0
   while [ "$i" -lt 150 ]; do
      if [ -z "$wayland_socket" ]; then
         for candidate in "$XDG_RUNTIME_DIR"/wayland-*; do
            case $candidate in
            *.lock)
               continue
               ;;
            esac
            if [ -S "$candidate" ]; then
               wayland_socket=$candidate
               break
            fi
         done
      fi
      if [ -z "$sway_socket" ]; then
         for candidate in "$XDG_RUNTIME_DIR"/sway-ipc.*.sock; do
            if [ -S "$candidate" ]; then
               sway_socket=$candidate
               break
            fi
         done
      fi
      if [ -n "$wayland_socket" ] && [ -n "$sway_socket" ]; then
         break
      fi
      if ! kill -0 "$sway_pid" 2>/dev/null; then
         break
      fi
      sleep 0.1
      i=$((i + 1))
   done

   if [ -z "$wayland_socket" ] || [ -z "$sway_socket" ]; then
      fail "sway did not create its Wayland and IPC sockets within 15 s"
   fi

   WAYLAND_DISPLAY=$(basename "$wayland_socket")
   SWAYSOCK=$sway_socket
   export WAYLAND_DISPLAY SWAYSOCK
   echo "ok sway is running (WAYLAND_DISPLAY=$WAYLAND_DISPLAY)"
}

# The first "y" of a workspace is its rect's top. The panel reserves the strip
# above it, so y >= 20 means the panel mapped.
panel_strip_y()
{
   # get_workspaces lists only real output workspaces; get_tree also contains
   # sway's internal __i3_scratch workspace at y=0, which must not be matched.
   # The first "y" in each workspace object is its rect y.
   "$SWAYMSG" -t get_workspaces 2>/dev/null \
      | sed -n 's/^ *"y": *\([0-9][0-9]*\).*/\1/p' \
      | head -n 1
}

wait_panel_wayland()
{
   i=0
   while [ "$i" -lt 200 ]; do
      y=$(panel_strip_y || true)
      case $y in
      ''|*[!0-9]*)
         ;;
      *)
         if [ "$y" -ge 20 ]; then
            panel_strip=$y
            return 0
         fi
         ;;
      esac
      if ! kill -0 "$panel_pid" 2>/dev/null; then
         return 1
      fi
      sleep 0.1
      i=$((i + 1))
   done
   return 1
}

check_panel_alive()
{
   label=$1
   if ! kill -0 "$panel_pid" 2>/dev/null; then
      fail "$label: the panel died"
   fi
   y=$(panel_strip_y || true)
   case $y in
   ''|*[!0-9]*)
      fail "$label: cannot read the workspace position from swaymsg"
      ;;
   esac
   if [ "$y" -lt 20 ]; then
      fail "$label: the panel strip disappeared"
   fi
}

# Number of layer surfaces sway created so far. Window mode adds one per open;
# GtkPopover does not use layer-shell, so the count proves which path ran.
# grep exits 1 on zero matches, hence the "|| true" and the empty default.
layer_count()
{
   count=$(grep -c 'new layer surface' "$sway_log" 2>/dev/null || true)
   echo "${count:-0}"
}

wait_layer_count()
{
   want=$1
   i=0
   while [ "$i" -lt 100 ]; do
      current=$(layer_count)
      if [ "$current" -ge "$want" ]; then
         return 0
      fi
      sleep 0.1
      i=$((i + 1))
   done
   return 1
}

capture()
{
   : >"$1"
   "$GRIM" "$1" >/dev/null 2>&1 || true
}

# Wait for popup pixels below the panel strip. The strip is excluded because
# hover state there changes independently of the popup.
wait_changed()
{
   i=0
   while [ "$i" -lt 100 ]; do
      capture "$shot"
      if [ -s "$shot" ]; then
         if out=$("$IMAGE_DIFF" --changed "$WAYLAND_PANEL_STRIP" 120 100 \
               "$baseline" "$shot" 2>/dev/null); then
            printf '%s\n' "$out"
            return 0
         fi
      fi
      sleep 0.1
      i=$((i + 1))
   done
   return 1
}

wait_unchanged()
{
   i=0
   while [ "$i" -lt 100 ]; do
      capture "$shot"
      if [ -s "$shot" ]; then
         if "$IMAGE_DIFF" --unchanged "$WAYLAND_PANEL_STRIP" "$baseline" "$shot" \
               >/dev/null 2>&1; then
            return 0
         fi
      fi
      sleep 0.1
      i=$((i + 1))
   done
   return 1
}

wayland_click()
{
   "$INPUT" click "$1" "$2" 1280 720 \
      || fail "popup-wayland-input could not click $1,$2"
}

check_wayland_bbox()
{
   label=$1
   box=$2
   case $box in
   bbox=*)
      box=${box#bbox=}
      ;;
   *)
      fail "$label: unexpected popup-image-diff output '$2'"
      ;;
   esac
   case $box in
   *,*,*,*)
      ;;
   *)
      fail "$label: no popup bounding box (got '$2')"
      ;;
   esac

   old_ifs=$IFS
   IFS=,
   box_text=$box
   set -- $box
   IFS=$old_ifs
   if [ "$#" -ne 4 ]; then
      fail "$label: malformed bounding box '$box_text'"
   fi
   box_x=$1
   box_y=$2
   box_w=$3
   box_h=$4

   if [ "$box_x" -lt 0 ]; then
      fail "$label: bbox x=$box_x is negative"
   fi
   if [ "$box_y" -lt "$WAYLAND_PANEL_STRIP" ] \
         || [ "$box_y" -gt "$((WAYLAND_PANEL_STRIP + 20))" ]; then
      fail "$label: bbox y=$box_y is not just below the panel (expected $WAYLAND_PANEL_STRIP..$((WAYLAND_PANEL_STRIP + 20)))"
   fi
   if [ "$box_x" -ge "$WAYLAND_BUTTON_RIGHT" ]; then
      fail "$label: bbox x=$box_x starts right of the plugin button (x < $WAYLAND_BUTTON_RIGHT expected)"
   fi
   if [ "$((box_x + box_w))" -lt "$((WAYLAND_BUTTON_RIGHT + 50))" ]; then
      fail "$label: bbox right edge $((box_x + box_w)) does not reach the button right edge (>= $((WAYLAND_BUTTON_RIGHT + 50)) expected)"
   fi
   if [ "$box_w" -lt 120 ] || [ "$box_h" -lt 100 ]; then
      fail "$label: popup ${box_w}x${box_h} is smaller than 120x100"
   fi
}

run_wayland()
{
   prepare_environment
   write_panel_config auto
   export GDK_BACKEND=wayland

   start_bus
   start_sway
   start_panel

   if ! wait_panel_wayland; then
      fail "the panel did not reserve the top strip within 20 s"
   fi
   echo "ok panel is up (workspace starts at y=$panel_strip)"

   baseline=$tmp/baseline.png
   baseline_check=$tmp/baseline-check.png
   shot=$tmp/shot.png
   # The first frame after the panel maps can be partially painted, so a single
   # baseline pair is not reliable: retry capture-compare until two consecutive
   # shots are stable below the panel strip.
   i=0
   baseline_ok=
   diff_out=
   while [ "$i" -lt 20 ]; do
      i=$((i + 1))
      capture "$baseline"
      sleep 0.3
      capture "$baseline_check"
      if [ -s "$baseline" ] && [ -s "$baseline_check" ] \
            && diff_out=$("$IMAGE_DIFF" --unchanged "$WAYLAND_PANEL_STRIP" \
               "$baseline" "$baseline_check" 2>"$tmp/diff.err"); then
         baseline_ok=1
         break
      fi
   done
   if [ -z "$baseline_ok" ]; then
      dump_log "$tmp/diff.err"
      fail "the baseline is not stable below y=$WAYLAND_PANEL_STRIP after 20 tries (last diff: '$diff_out')"
   fi
   echo "ok baseline screenshot is stable and shows no popup below y=$WAYLAND_PANEL_STRIP"

   # 1. window mode, driven through the real remote-event path once
   xfconf_set_mode window
   sleep 0.5
   layer_before=$(layer_count)
   sh "$POPUP_SCRIPT" || fail "the popup script failed in window mode"
   wait_layer_count "$((layer_before + 1))" \
      || fail "window mode did not create a new layer surface for the remote event"
   bbox=$(wait_changed) || fail "window-mode popup pixels did not appear"
   check_wayland_bbox "remote-event window popup" "$bbox"
   echo "ok remote-event window popup created a layer surface and visible pixels ($bbox)"

   wayland_click 20 13
   wait_unchanged || fail "window popup pixels did not disappear after the button click"
   sleep 0.2
   echo "ok button click closed the window popup"

   # 2. one-click reopen
   layer_before=$(layer_count)
   wayland_click 20 13
   wait_layer_count "$((layer_before + 1))" \
      || fail "one-click reopen did not create a new layer surface"
   bbox=$(wait_changed) || fail "reopened window popup pixels did not appear"
   check_wayland_bbox "reopened window popup" "$bbox"
   echo "ok one-click reopen created another layer surface and popup pixels"
   wayland_click 20 13
   wait_unchanged || fail "the reopened window popup did not close"
   sleep 0.2
   echo "ok button click closed the reopened window popup"

   # 3. a mode change to popover while the window popup is open is deferred
   layer_before=$(layer_count)
   wayland_click 20 13
   wait_layer_count "$((layer_before + 1))" \
      || fail "could not open the window popup for the deferred check"
   bbox=$(wait_changed) || fail "window popup pixels did not appear for the deferred check"

   xfconf_set_mode popover
   sleep 0.6
   if [ "$(layer_count)" -ne "$((layer_before + 1))" ]; then
      fail "switching to popover while the window popup was open changed the layer surface count"
   fi
   capture "$shot"
   if ! "$IMAGE_DIFF" --changed "$WAYLAND_PANEL_STRIP" 120 100 \
         "$baseline" "$shot" >/dev/null 2>&1; then
      fail "the open window popup disappeared when popup-mode changed to popover (expected deferred)"
   fi
   echo "ok popup-mode change to popover is deferred while the window popup is open"
   wayland_click 20 13
   wait_unchanged || fail "the deferred window popup did not close"
   sleep 0.2
   echo "ok button click closed the deferred window popup"

   # 4. after the deferred close, popover mode must not create a layer surface,
   #    and the plugin must survive; visibility is not asserted
   layer_before=$(layer_count)
   wayland_click 20 13
   sleep 0.8
   if [ "$(layer_count)" -ne "$layer_before" ]; then
      fail "popover mode created a layer surface after the deferred close"
   fi
   check_panel_alive "popover mode after deferred close"
   echo "ok popover mode after the deferred close created no layer surface (visibility not asserted)"
   wayland_click 20 13
   sleep 0.3

   # 5. one more window-mode open and close proves the click path still works
   xfconf_set_mode window
   sleep 0.5
   layer_before=$(layer_count)
   wayland_click 20 13
   wait_layer_count "$((layer_before + 1))" \
      || fail "window mode did not create a layer surface after leaving popover"
   bbox=$(wait_changed) || fail "window-mode pixels did not return"
   check_wayland_bbox "window popup after popover" "$bbox"
   wayland_click 20 13
   wait_unchanged || fail "the final window popup did not close"

   # 6. auto mode selects the popover path. Run last, after the window-mode
   #    clicks above proved that (20,13) reaches the plugin. GtkPopover is not
   #    visibly rendered under the headless test server, so assert only that no
   #    layer surface (the window-mode mechanism) was created; visibility is a
   #    manual desktop check.
   xfconf_set_mode auto
   sleep 0.5
   layer_before=$(layer_count)
   wayland_click 20 13
   sleep 0.8
   if [ "$(layer_count)" -ne "$layer_before" ]; then
      fail "auto mode created a layer surface although the popover path should have been selected"
   fi
   check_panel_alive "auto mode"
   echo "ok auto mode created no layer surface, popover routing selected (visibility not asserted)"
   wayland_click 20 13
   sleep 0.3

   check_panel_alive "end of test"
   echo "ok panel and plugin are still alive"
   echo "popup-panel: all Wayland assertions passed"
}

case $test_mode in
x11)
   run_x11
   ;;
wayland)
   run_wayland
   ;;
esac
