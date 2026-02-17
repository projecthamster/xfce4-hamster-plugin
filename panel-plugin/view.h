/*  xfce4-hamster-plugin
 *
 *  Copyright (c) 2014-2023 Hakan Erduman <hakan@erduman.de>
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
 *  along with this program; If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <glib.h>
#include <gtk/gtk.h>
#include <libxfce4panel/libxfce4panel.h>

typedef struct _HamsterView HamsterView;

/* Init & Finalize */
HamsterView*
hamster_view_init(XfcePanelPlugin*);

void
hamster_view_finalize(HamsterView*);

/* config.c */
void
config_show(XfcePanelPlugin *plugin, HamsterView *view);

/* view.c */
void
hview_popup_show(HamsterView *view, gboolean atPointer);
