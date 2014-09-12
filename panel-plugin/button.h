/*  xfce4-places-plugin
 *
 *  Provides the widget that sits on the panel
 *
 *  Note that, while this extends GtkToggleButton, much of the gtk_button_*()
 *  functions shouldn't be used.
 *
 *  Copyright (c) 2007-2008 Diego Ongaro <ongardie@gmail.com>
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
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef _XFCE_PANEL_PLACES_BUTTON_H
#define _XFCE_PANEL_PLACES_BUTTON_H

#include <gtk/gtk.h>

#define PLACES_TYPE_BUTTON             (places_button_get_type ())
#define PLACES_BUTTON(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), PLACES_TYPE_BUTTON, PlacesButton))
#define PLACES_BUTTON_CLASS(obj)       (G_TYPE_CHECK_CLASS_CAST ((obj), PLACES_TYPE_BUTTON,  PlacesButtonClass))
#define PLACES_IS_BUTTON(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PLACES_TYPE_BUTTON))
#define PLACES_IS_BUTTON_CLASS(obj)    (G_TYPE_CHECK_CLASS_TYPE ((obj), PLACES_TYPE_BUTTON))
#define PLACES_BUTTON_GET_CLASS        (G_TYPE_INSTANCE_GET_CLASS ((obj), PLACES_TYPE_BUTTON, PlacesButtonClass))

typedef struct _PlacesButton            PlacesButton;
typedef struct _PlacesButtonClass       PlacesButtonClass;

typedef GdkPixbuf* (places_button_image_pixbuf_factory) (int size);

struct _PlacesButton
{
    GtkToggleButton parent;

    /* private */
    XfcePanelPlugin *plugin;
    GtkWidget *alignment;
    GtkWidget *box;
    GtkWidget *image;
    GtkWidget *label;
    gchar *label_text;
    places_button_image_pixbuf_factory *pixbuf_factory;
    gint plugin_size;
    gulong style_set_id;
    gulong screen_changed_id;
};

struct _PlacesButtonClass
{
    GtkToggleButtonClass parent_class;
};

GType
places_button_get_type();

GtkWidget*
places_button_new();

void
places_button_set_label(PlacesButton*, const gchar *label);

const gchar*
places_button_get_label(PlacesButton*);

#endif
/* vim: set ai et tabstop=4: */
