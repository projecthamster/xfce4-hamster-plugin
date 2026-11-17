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

#include <string.h>

#include "popup-decision.h"

/* ASCII-only lower casing: explicit 'A'..'Z' fold, so the comparison does not
 * depend on the locale's LC_CTYPE table. */
static unsigned char ascii_lower(unsigned char c)
{
   if (c >= (unsigned char) 'A' && c <= (unsigned char) 'Z')
   {
      return (unsigned char) (c + ('a' - 'A'));
   }
   return c;
}

/* ASCII-only case folding comparison, locale independent. */
static int ascii_casecmp(const char *a, const char *b)
{
   unsigned char ca;
   unsigned char cb;

   while (*a != '\0' && *b != '\0')
   {
      ca = ascii_lower((unsigned char) *a);
      cb = ascii_lower((unsigned char) *b);
      if (ca != cb)
      {
         return (int) ca - (int) cb;
      }
      a++;
      b++;
   }
   return (int) (unsigned char) *a - (int) (unsigned char) *b;
}

HviewPopupMode hview_popup_mode_from_string(const char *mode)
{
   if (mode == NULL || *mode == '\0')
   {
      return HVIEW_POPUP_MODE_AUTO;
   }
   if (ascii_casecmp(mode, "popover") == 0)
   {
      return HVIEW_POPUP_MODE_POPOVER;
   }
   if (ascii_casecmp(mode, "window") == 0)
   {
      return HVIEW_POPUP_MODE_WINDOW;
   }
   return HVIEW_POPUP_MODE_AUTO;
}

int hview_display_name_is_wayland(const char *display_name)
{
   if (display_name == NULL || *display_name == '\0')
   {
      return 0;
   }
   return strncmp(display_name, "wayland", 7) == 0;
}

HviewPopupKind hview_popup_kind(const HviewPopupRequest *request)
{
   int mode    = HVIEW_POPUP_MODE_AUTO;
   int pointer = 0;
   int wayland = 0;
   int panel   = 0;

   if (request != NULL)
   {
      mode    = request->mode;
      pointer = request->at_pointer;
      wayland = request->wayland;
      panel   = request->panel_popup_window;
   }

   if (wayland)
   {
      if (mode == HVIEW_POPUP_MODE_WINDOW)
      {
         return panel ? HVIEW_POPUP_KIND_PANEL_WINDOW : HVIEW_POPUP_KIND_WINDOW;
      }
      /* AUTO, POPOVER and unknown modes: a toplevel cannot be positioned on
       * Wayland, so always use the popover. at_pointer is ignored. */
      return HVIEW_POPUP_KIND_POPOVER;
   }

   /* X11: WINDOW mode always uses a toplevel; AUTO, POPOVER and unknown modes
    * use a toplevel only when the popup was requested at the pointer. */
   if (mode == HVIEW_POPUP_MODE_WINDOW || pointer)
   {
      return HVIEW_POPUP_KIND_WINDOW;
   }
   return HVIEW_POPUP_KIND_POPOVER;
}
