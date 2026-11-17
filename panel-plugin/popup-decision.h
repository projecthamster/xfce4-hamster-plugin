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

#pragma once

/* Pure, GTK-free popup mode selection. This unit must stay buildable on its
 * own (plain C, no other includes) so it can be unit tested without a
 * display. No glib, gtk or libxfce4panel includes here. */

/* How the wiki/UI asks for the popup to be shown. */
typedef enum
{
   HVIEW_POPUP_MODE_AUTO    = 0, /* decide from the backend and the request */
   HVIEW_POPUP_MODE_POPOVER,     /* prefer an in-panel GtkPopover */
   HVIEW_POPUP_MODE_WINDOW       /* prefer a toplevel GtkWindow */
} HviewPopupMode;

/* What should actually be created and shown. */
typedef enum
{
   HVIEW_POPUP_KIND_POPOVER      = 0, /* GtkPopover attached to the button */
   HVIEW_POPUP_KIND_WINDOW,           /* plain toplevel GtkWindow */
   HVIEW_POPUP_KIND_PANEL_WINDOW      /* toplevel positioned by the panel */
} HviewPopupKind;

/* All inputs of the popup decision. */
typedef struct
{
   int mode;               /* HviewPopupMode requested by the user/config */
   int at_pointer;         /* boolean: popup requested at the pointer (-p / --pointer) */
   int wayland;            /* boolean: GDK backend is Wayland */
   int panel_popup_window; /* boolean: built against libxfce4panel >= 4.19 */
} HviewPopupRequest;

/* Parse a configured mode name.
 *
 * NULL, an empty string, "auto" and anything unrecognised yield
 * HVIEW_POPUP_MODE_AUTO. "popover" and "window" are matched ASCII
 * case-insensitively and yield the corresponding mode. */
HviewPopupMode
hview_popup_mode_from_string(const char *mode);

/* Decide which kind of popup to show for the given request.
 *
 * NULL is treated like an all-zero request (auto mode, no pointer, X11).
 * On Wayland only WINDOW mode can produce a window, and only when
 * panel_popup_window is set; every other Wayland request uses a popover.
 * On X11 WINDOW mode and at_pointer produce a window, and panel_popup_window
 * never produces a panel window. The function is pure: no globals, no I/O. */
HviewPopupKind
hview_popup_kind(const HviewPopupRequest *request);

/* Return non-zero when the GDK display name names a Wayland display.
 *
 * NULL and empty names yield 0. Otherwise the check is a case-sensitive
 * "wayland" prefix test, matching g_str_has_prefix(). */
int
hview_display_name_is_wayland(const char *display_name);
