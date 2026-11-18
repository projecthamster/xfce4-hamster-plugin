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

/* Display-free unit test for panel-plugin/popup-decision.c.
 *
 * Table driven, plain C, no test framework and no glib/gtk: it only links the
 * pure decision unit, so it runs anywhere. Every case prints "ok <case>" or
 * "FAIL <case>"; any failure makes the process exit non-zero. */

#include <stdio.h>
#include <string.h>

#include "popup-decision.h"

static int checks = 0;
static int failures = 0;

#define CHECK_INT(case_name, actual, expected)                                \
   do {                                                                       \
      long actual_value_ = (long) (actual);                                   \
      long expected_value_ = (long) (expected);                               \
      checks++;                                                               \
      if (actual_value_ == expected_value_)                                   \
      {                                                                       \
         printf("ok %s\n", (case_name));                                      \
      }                                                                       \
      else                                                                    \
      {                                                                       \
         failures++;                                                          \
         printf("FAIL %s (expected %ld, got %ld)\n", (case_name),             \
                expected_value_, actual_value_);                              \
      }                                                                       \
   } while (0)

static const char *mode_name(HviewPopupMode mode)
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

typedef struct
{
   const char *input;
   HviewPopupMode expected;
   const char *label;
} ModeFromStringCase;

static const ModeFromStringCase mode_from_string_cases[] = {
   { NULL,        HVIEW_POPUP_MODE_AUTO,    "mode_from_string(NULL)" },
   { "",          HVIEW_POPUP_MODE_AUTO,    "mode_from_string(\"\")" },
   { "auto",      HVIEW_POPUP_MODE_AUTO,    "mode_from_string(\"auto\")" },
   { "AUTO",      HVIEW_POPUP_MODE_AUTO,    "mode_from_string(\"AUTO\")" },
   { "popover",   HVIEW_POPUP_MODE_POPOVER, "mode_from_string(\"popover\")" },
   { "POPOVER",   HVIEW_POPUP_MODE_POPOVER, "mode_from_string(\"POPOVER\")" },
   { "popOver",   HVIEW_POPUP_MODE_POPOVER, "mode_from_string(\"popOver\")" },
   { "window",    HVIEW_POPUP_MODE_WINDOW,  "mode_from_string(\"window\")" },
   { "Window",    HVIEW_POPUP_MODE_WINDOW,  "mode_from_string(\"Window\")" },
   { "bogus",     HVIEW_POPUP_MODE_AUTO,    "mode_from_string(\"bogus\")" },
};

typedef struct
{
   const char *input;
   int expected;
   const char *label;
} WaylandNameCase;

static const WaylandNameCase wayland_name_cases[] = {
   { NULL,        0, "wayland_name(NULL)" },
   { "",          0, "wayland_name(\"\")" },
   { "wayland-0", 1, "wayland_name(\"wayland-0\")" },
   { "wayland",   1, "wayland_name(\"wayland\")" },
   { ":0",        0, "wayland_name(\":0\")" },
   { "localhost:0", 0, "wayland_name(\"localhost:0\")" },
   { "Wayland-1", 0, "wayland_name(\"Wayland-1\")" },
};

/* The decision table, written out literally so the test does not repeat the
 * implementation. Wayland auto/window modes depend on the panel capability. */
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
     HVIEW_POPUP_KIND_POPOVER, HVIEW_POPUP_KIND_PANEL_WINDOW },
   { HVIEW_POPUP_MODE_AUTO, 1, HVIEW_POPUP_KIND_WINDOW,
     HVIEW_POPUP_KIND_POPOVER, HVIEW_POPUP_KIND_PANEL_WINDOW },
   { HVIEW_POPUP_MODE_POPOVER, 0, HVIEW_POPUP_KIND_POPOVER,
     HVIEW_POPUP_KIND_POPOVER, HVIEW_POPUP_KIND_POPOVER },
   { HVIEW_POPUP_MODE_POPOVER, 1, HVIEW_POPUP_KIND_WINDOW,
     HVIEW_POPUP_KIND_POPOVER, HVIEW_POPUP_KIND_POPOVER },
   { HVIEW_POPUP_MODE_WINDOW, 0, HVIEW_POPUP_KIND_WINDOW,
     HVIEW_POPUP_KIND_WINDOW, HVIEW_POPUP_KIND_PANEL_WINDOW },
   { HVIEW_POPUP_MODE_WINDOW, 1, HVIEW_POPUP_KIND_WINDOW,
     HVIEW_POPUP_KIND_WINDOW, HVIEW_POPUP_KIND_PANEL_WINDOW },
};

static void test_mode_from_string(void)
{
   size_t i;

   for (i = 0; i < sizeof(mode_from_string_cases) / sizeof(mode_from_string_cases[0]); i++)
   {
      const ModeFromStringCase *c = &mode_from_string_cases[i];
      HviewPopupMode actual = hview_popup_mode_from_string(c->input);

      CHECK_INT(c->label, actual, c->expected);
   }
}

static void test_wayland_display_name(void)
{
   size_t i;

   for (i = 0; i < sizeof(wayland_name_cases) / sizeof(wayland_name_cases[0]); i++)
   {
      const WaylandNameCase *c = &wayland_name_cases[i];
      int actual = hview_display_name_is_wayland(c->input);

      CHECK_INT(c->label, actual, c->expected);
   }
}

static void test_popup_kind(void)
{
   int wayland;
   int panel;
   size_t i;
   char label[160];
   HviewPopupRequest request;

   for (wayland = 0; wayland <= 1; wayland++)
   {
      for (panel = 0; panel <= 1; panel++)
      {
         for (i = 0; i < sizeof(kind_cases) / sizeof(kind_cases[0]); i++)
         {
            const KindCase *c = &kind_cases[i];
            HviewPopupKind expected;
            HviewPopupKind actual;

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

            actual = hview_popup_kind(&request);

            snprintf(label, sizeof label, "kind %s panel=%d mode=%s pointer=%d -> %s",
                     wayland ? "wayland" : "x11", panel, mode_name(c->mode),
                     c->at_pointer, kind_name(expected));
            CHECK_INT(label, actual, expected);
         }

         /* NULL is an all-zero X11 auto request. */
         snprintf(label, sizeof label, "kind NULL request panel=%d -> popover", panel);
         CHECK_INT(label, hview_popup_kind(NULL), HVIEW_POPUP_KIND_POPOVER);
      }
   }

   /* Unknown mode 42 is treated like auto on both backends. */
   request.mode = 42;
   request.at_pointer = 0;
   request.wayland = 0;
   request.panel_popup_window = 0;
   CHECK_INT("kind x11 mode=42 pointer=0 -> popover",
             hview_popup_kind(&request), HVIEW_POPUP_KIND_POPOVER);
   request.at_pointer = 1;
   CHECK_INT("kind x11 mode=42 pointer=1 -> window",
             hview_popup_kind(&request), HVIEW_POPUP_KIND_WINDOW);
   request.wayland = 1;
   CHECK_INT("kind wayland mode=42 pointer=1 panel=0 -> popover",
             hview_popup_kind(&request), HVIEW_POPUP_KIND_POPOVER);
   request.panel_popup_window = 1;
   CHECK_INT("kind wayland mode=42 pointer=1 panel=1 -> panel-window",
             hview_popup_kind(&request), HVIEW_POPUP_KIND_PANEL_WINDOW);
}

int main(void)
{
   test_mode_from_string();
   test_wayland_display_name();
   test_popup_kind();

   printf("%d checks, %d failure(s)\n", checks, failures);
   return failures == 0 ? 0 : 1;
}
