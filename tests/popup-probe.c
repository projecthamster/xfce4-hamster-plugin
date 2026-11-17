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

/* Real-display probe for the popup decision and the GTK popup widgets.
 *
 * Run it on a real GDK backend, typically from tests/popup-runtime.sh:
 *     popup-probe --expect x11|wayland
 *
 * It checks that the backend is detected from the GDK display name, replays
 * the decision table for that backend, and shows a GtkPopover and an
 * undecorated utility toplevel to prove both map. It never calls into
 * libxfce4panel, so it can run under plain Xvfb or headless sway; the panel
 * anchored popup itself needs a live xfce4-panel and is out of scope. */

#include <stdio.h>
#include <string.h>

#include <gtk/gtk.h>

#include "popup-decision.h"
#include "view.h"

static int checks = 0;
static int failures = 0;

static const char *mode_name(int mode)
{
   switch (mode)
   {
   case HVIEW_POPUP_MODE_AUTO:
      return "auto";
   case HVIEW_POPUP_MODE_POPOVER:
      return "popover";
   case HVIEW_POPUP_MODE_WINDOW:
      return "window";
   }
   return "unknown";
}

static const char *kind_name(HviewPopupKind kind)
{
   switch (kind)
   {
   case HVIEW_POPUP_KIND_POPOVER:
      return "popover";
   case HVIEW_POPUP_KIND_WINDOW:
      return "window";
   case HVIEW_POPUP_KIND_PANEL_WINDOW:
      return "panel-window";
   }
   return "unknown";
}

static void check_kind(const char *label, HviewPopupKind actual, HviewPopupKind expected)
{
   checks++;
   if (actual == expected)
   {
      printf("ok %s\n", label);
   }
   else
   {
      failures++;
      printf("FAIL %s (expected %s, got %s)\n", label,
             kind_name(expected), kind_name(actual));
   }
}

static void check_bool(const char *label, int actual, int expected)
{
   checks++;
   if ((actual != 0) == (expected != 0))
   {
      printf("ok %s\n", label);
   }
   else
   {
      failures++;
      printf("FAIL %s (expected %d, got %d)\n", label, expected, actual);
   }
}

/* The decision table, written out literally. */
typedef struct
{
   int mode;
   int at_pointer;
   HviewPopupKind x11_kind;
   HviewPopupKind wayland_kind;
   HviewPopupKind wayland_panel_kind;
} KindCase;

static const KindCase kind_cases[] = {
   { HVIEW_POPUP_MODE_AUTO, 0, HVIEW_POPUP_KIND_POPOVER,
     HVIEW_POPUP_KIND_POPOVER, HVIEW_POPUP_KIND_POPOVER },
   { HVIEW_POPUP_MODE_AUTO, 1, HVIEW_POPUP_KIND_WINDOW,
     HVIEW_POPUP_KIND_POPOVER, HVIEW_POPUP_KIND_POPOVER },
   { HVIEW_POPUP_MODE_POPOVER, 0, HVIEW_POPUP_KIND_POPOVER,
     HVIEW_POPUP_KIND_POPOVER, HVIEW_POPUP_KIND_POPOVER },
   { HVIEW_POPUP_MODE_POPOVER, 1, HVIEW_POPUP_KIND_WINDOW,
     HVIEW_POPUP_KIND_POPOVER, HVIEW_POPUP_KIND_POPOVER },
   { HVIEW_POPUP_MODE_WINDOW, 0, HVIEW_POPUP_KIND_WINDOW,
     HVIEW_POPUP_KIND_WINDOW, HVIEW_POPUP_KIND_PANEL_WINDOW },
   { HVIEW_POPUP_MODE_WINDOW, 1, HVIEW_POPUP_KIND_WINDOW,
     HVIEW_POPUP_KIND_WINDOW, HVIEW_POPUP_KIND_PANEL_WINDOW },
};

static void check_decisions(int wayland)
{
   int panel;
   size_t i;
   char label[160];
   HviewPopupRequest request;

   for (panel = 0; panel <= 1; panel++)
   {
      for (i = 0; i < sizeof(kind_cases) / sizeof(kind_cases[0]); i++)
      {
         const KindCase *c = &kind_cases[i];
         HviewPopupKind expected;

         if (!wayland)
         {
            expected = c->x11_kind;
         }
         else if (panel)
         {
            expected = c->wayland_panel_kind;
         }
         else
         {
            expected = c->wayland_kind;
         }

         request.mode = c->mode;
         request.at_pointer = c->at_pointer;
         request.wayland = wayland;
         request.panel_popup_window = panel;

         snprintf(label, sizeof label, "kind %s panel=%d mode=%s pointer=%d -> %s",
                  wayland ? "wayland" : "x11", panel, mode_name(c->mode),
                  c->at_pointer, kind_name(expected));
         check_kind(label, hview_popup_kind(&request), expected);
      }

      snprintf(label, sizeof label, "kind %s panel=%d NULL request -> popover",
               wayland ? "wayland" : "x11", panel);
      check_kind(label, hview_popup_kind(NULL), HVIEW_POPUP_KIND_POPOVER);
   }

   /* Unknown mode 42 behaves like auto. */
   request.mode = 42;
   request.at_pointer = 0;
   request.wayland = wayland;
   request.panel_popup_window = 0;
   snprintf(label, sizeof label, "kind %s mode=42 pointer=0 -> popover",
            wayland ? "wayland" : "x11");
   check_kind(label, hview_popup_kind(&request), HVIEW_POPUP_KIND_POPOVER);
   request.at_pointer = 1;
   snprintf(label, sizeof label, "kind %s mode=42 pointer=1 -> %s",
            wayland ? "wayland" : "x11",
            wayland ? "popover" : "window");
   check_kind(label, hview_popup_kind(&request),
              wayland ? HVIEW_POPUP_KIND_POPOVER : HVIEW_POPUP_KIND_WINDOW);
}

static gboolean quit_main_loop(gpointer data)
{
   (void) data;
   gtk_main_quit();
   return G_SOURCE_REMOVE;
}

/* Let GTK process events for roughly the given time. */
static void pump_milliseconds(unsigned milliseconds)
{
   g_timeout_add(milliseconds, quit_main_loop, NULL);
   gtk_main();
}

/* Pump the main loop in slices until the widget is mapped or the deadline
 * expires, then return the final mapped state. Mapping is asynchronous on both
 * backends, so a fixed pump would be flaky on a slow machine. */
static int pump_until_mapped(GtkWidget *widget)
{
   gint64 deadline = g_get_monotonic_time()
                     + (gint64) 2000 * G_TIME_SPAN_MILLISECOND;

   while (!gtk_widget_get_mapped(widget)
          && g_get_monotonic_time() < deadline)
   {
      pump_milliseconds(20);
   }
   return gtk_widget_get_mapped(widget) != FALSE;
}

static void probe_widgets(void)
{
   GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
   GtkWidget *button = gtk_button_new_with_label("hamster popup probe");
   GtkWidget *popover;
   GtkWidget *label;
   GtkWidget *utility;

   gtk_window_set_title(GTK_WINDOW(window), "popup-probe");
   gtk_window_set_default_size(GTK_WINDOW(window), 240, 120);
   gtk_container_add(GTK_CONTAINER(window), button);
   gtk_widget_show_all(window);
   pump_milliseconds(200);

   check_bool("toplevel mapped", pump_until_mapped(window), TRUE);
   check_bool("toplevel button mapped", gtk_widget_get_mapped(button), TRUE);

   popover = gtk_popover_new(button);
   gtk_popover_set_position(GTK_POPOVER(popover), GTK_POS_BOTTOM);
   gtk_popover_set_modal(GTK_POPOVER(popover), FALSE);
   label = gtk_label_new("hamster popover probe");
   gtk_container_add(GTK_CONTAINER(popover), label);
   gtk_widget_show_all(popover);
   gtk_popover_popup(GTK_POPOVER(popover));
   check_bool("popover mapped", pump_until_mapped(popover), TRUE);
   gtk_popover_popdown(GTK_POPOVER(popover));
   pump_milliseconds(50);

   /* The legacy/unmanaged fallback: an undecorated utility toplevel. */
   utility = gtk_window_new(GTK_WINDOW_TOPLEVEL);
   gtk_window_set_decorated(GTK_WINDOW(utility), FALSE);
   gtk_window_set_type_hint(GTK_WINDOW(utility), GDK_WINDOW_TYPE_HINT_UTILITY);
   gtk_container_add(GTK_CONTAINER(utility), gtk_label_new("hamster utility probe"));
   gtk_widget_show_all(utility);
   check_bool("utility toplevel mapped", pump_until_mapped(utility), TRUE);
   gtk_widget_destroy(utility);

   gtk_widget_destroy(window);
}

int main(int argc, char **argv)
{
   GdkDisplay *display;
   const char *display_name;
   const char *expect;
   int is_wayland;

   if (argc != 3 || strcmp(argv[1], "--expect") != 0
       || (strcmp(argv[2], "x11") != 0 && strcmp(argv[2], "wayland") != 0))
   {
      fprintf(stderr, "usage: %s --expect x11|wayland\n", argv[0]);
      return 2;
   }
   expect = argv[2];

   gtk_init(&argc, &argv);

   display = gdk_display_get_default();
   if (display == NULL)
   {
      printf("FAIL display: gdk_display_get_default() returned NULL\n");
      return 1;
   }

   display_name = gdk_display_get_name(display);
   if (display_name == NULL)
   {
      display_name = "";
   }

   is_wayland = hview_display_name_is_wayland(display_name) != 0;
   printf("display=%s wayland_by_name=%d expect=%s\n",
          display_name, is_wayland, expect);
   check_bool("GDK backend matches --expect", is_wayland,
              strcmp(expect, "wayland") == 0);

   check_decisions(is_wayland);
   probe_widgets();

   printf("scope: panel anchoring, autohide blocking, click-outside and "
          "xfce_panel_plugin_popup_window() are NOT exercised "
          "(they need a live xfce4-panel)\n");
   printf("scope: at-pointer positioning is not exercised, only the popup "
          "kind decision\n");
   if (is_wayland && !HVIEW_CAN_PANEL_POPUP_WINDOW)
   {
      printf("scope: built without libxfce4panel >= 4.19, window mode is the "
             "unmanaged toplevel fallback\n");
   }

   printf("%d checks, %d failure(s)\n", checks, failures);
   return failures == 0 ? 0 : 1;
}
