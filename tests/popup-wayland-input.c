/*  xfce4-hamster-plugin
 *
 *  Copyright (c) 2026 Hakan Erduman <hakan@erduman.de>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; If not, see <https://www.gnu.org/licenses/>.
 */

/* Minimal Wayland pointer client for the live panel test.
 *
 * wlrctl cannot keep one virtual pointer device alive across a move and a
 * click, so this speaks sway's zwlr_virtual_pointer_manager_v1 directly: one
 * device, absolute motion to the target, then a click. It has no GTK and only
 * needs wayland-client, so it can run inside the headless compositor of
 * popup-panel.sh.
 *
 * usage: popup-wayland-input click X Y [W H]
 */

#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <wayland-client.h>

#include "wlr-virtual-pointer-unstable-v1-client-protocol.h"

/* linux/input-event-codes.h is not guaranteed in the build image; BTN_LEFT
 * has been 0x110 since the input protocol was introduced. */
#define BTN_LEFT 0x110

static struct wl_seat *seat;
static struct zwlr_virtual_pointer_manager_v1 *manager;

static void seat_capabilities(void *data, struct wl_seat *wl_seat,
                              uint32_t capabilities)
{
   (void) data;
   (void) wl_seat;
   (void) capabilities;
}

static void seat_name(void *data, struct wl_seat *wl_seat, const char *name)
{
   (void) data;
   (void) wl_seat;
   (void) name;
}

static const struct wl_seat_listener seat_listener = {
   seat_capabilities,
   seat_name,
};

static void registry_global(void *data, struct wl_registry *registry,
                            uint32_t name, const char *interface,
                            uint32_t version)
{
   (void) data;

   if (strcmp(interface, wl_seat_interface.name) == 0 && seat == NULL)
   {
      /* Only a seat handle is needed; its events are irrelevant. */
      seat = wl_registry_bind(registry, name, &wl_seat_interface, 1);
      wl_seat_add_listener(seat, &seat_listener, NULL);
   }
   else if (strcmp(interface,
                   zwlr_virtual_pointer_manager_v1_interface.name) == 0
            && manager == NULL)
   {
      /* Version 2 adds create_virtual_pointer_with_output, which a plain
       * click does not need; bind at most 2 so both work. */
      uint32_t bind_version = version < 2 ? version : 2;

      manager = wl_registry_bind(
         registry, name, &zwlr_virtual_pointer_manager_v1_interface,
         bind_version);
   }
}

static void registry_global_remove(void *data, struct wl_registry *registry,
                                   uint32_t name)
{
   (void) data;
   (void) registry;
   (void) name;
}

static const struct wl_registry_listener registry_listener = {
   registry_global,
   registry_global_remove,
};

static uint32_t monotonic_millis(void)
{
   struct timespec ts;

   clock_gettime(CLOCK_MONOTONIC, &ts);
   return (uint32_t) (ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

static void sleep_millis(long milliseconds)
{
   struct timespec ts;

   ts.tv_sec = milliseconds / 1000;
   ts.tv_nsec = (milliseconds % 1000) * 1000000L;
   while (nanosleep(&ts, &ts) == -1 && errno == EINTR)
   {
   }
}

static int parse_uint(const char *text, uint32_t *value)
{
   char *end = NULL;
   unsigned long parsed;

   errno = 0;
   parsed = strtoul(text, &end, 10);
   if (errno != 0 || end == text || *end != '\0' || parsed > UINT32_MAX)
   {
      return 0;
   }
   *value = (uint32_t) parsed;
   return 1;
}

/* A failed roundtrip means the compositor rejected a request; there is no
 * meaningful way to continue, so report and leave. */
static void roundtrip(struct wl_display *display)
{
   if (wl_display_roundtrip(display) < 0)
   {
      fprintf(stderr, "popup-wayland-input: display roundtrip failed\n");
      exit(1);
   }
}

int main(int argc, char **argv)
{
   struct wl_display *display;
   struct wl_registry *registry;
   struct zwlr_virtual_pointer_v1 *pointer;
   uint32_t x;
   uint32_t y;
   uint32_t width = 1280;
   uint32_t height = 720;

   if (argc != 4 && argc != 6)
   {
      fprintf(stderr, "usage: %s click X Y [W H]\n", argv[0]);
      return 2;
   }
   if (strcmp(argv[1], "click") != 0 || !parse_uint(argv[2], &x)
       || !parse_uint(argv[3], &y)
       || (argc == 6 && (!parse_uint(argv[4], &width)
                         || !parse_uint(argv[5], &height))))
   {
      fprintf(stderr, "usage: %s click X Y [W H]\n", argv[0]);
      return 2;
   }

   display = wl_display_connect(NULL);
   if (display == NULL)
   {
      fprintf(stderr, "popup-wayland-input: cannot connect to the default "
                      "Wayland display (is WAYLAND_DISPLAY set?)\n");
      return 1;
   }

   registry = wl_display_get_registry(display);
   wl_registry_add_listener(registry, &registry_listener, NULL);
   /* Two roundtrips: the registry batch may arrive in several events and the
    * manager must exist before the virtual pointer is created. */
   roundtrip(display);
   roundtrip(display);

   if (manager == NULL)
   {
      fprintf(stderr, "popup-wayland-input: the compositor does not advertise "
                      "zwlr_virtual_pointer_manager_v1\n");
      return 1;
   }

   pointer = zwlr_virtual_pointer_manager_v1_create_virtual_pointer(
      manager, seat);
   roundtrip(display);

   zwlr_virtual_pointer_v1_motion_absolute(pointer, monotonic_millis(), x, y,
                                           width, height);
   zwlr_virtual_pointer_v1_frame(pointer);
   roundtrip(display);
   /* Let the compositor settle the pointer before the press; a compositor
    * that coalesces a move and a click reports the click at the old spot
    * otherwise. */
   sleep_millis(300);

   zwlr_virtual_pointer_v1_button(pointer, monotonic_millis(), BTN_LEFT,
                                  WL_POINTER_BUTTON_STATE_PRESSED);
   zwlr_virtual_pointer_v1_frame(pointer);
   roundtrip(display);
   sleep_millis(80);

   zwlr_virtual_pointer_v1_button(pointer, monotonic_millis(), BTN_LEFT,
                                  WL_POINTER_BUTTON_STATE_RELEASED);
   zwlr_virtual_pointer_v1_frame(pointer);
   roundtrip(display);

   zwlr_virtual_pointer_v1_destroy(pointer);
   zwlr_virtual_pointer_manager_v1_destroy(manager);
   if (seat != NULL)
   {
      wl_seat_destroy(seat);
   }
   wl_registry_destroy(registry);
   roundtrip(display);

   wl_display_disconnect(display);
   return 0;
}
