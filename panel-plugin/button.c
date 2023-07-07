/*  taken from xfce4-places-plugin, heavily modified
 *
 *  Provides the widget that sits on the panel
 *
 *  Note that, while this extends GtkToggleButton, much of the gtk_button_*()
 *  functions shouldn't be used.
 *
 *  Copyright (c) 2007-2008 Diego Ongaro <ongardie@gmail.com>
 *
 *  Some code adapted from libxfce4panel for the togglebutton configuration
 *    (xfce-panel-convenience.c)
 *    Copyright (c) 2006-2007 Jasper Huijsmans <jasper@xfce.org>
 *
 *  Some code adapted from gtk+ for the properties
 *    (gtkbutton.c)
 *    Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *    Modified by the GTK+ Team and others 1997-2001
 *
 *  May also contain code adapted from:
 *   - notes plugin
 *     panel-plugin.c - (xfce4-panel plugin for temporary notes)
 *     Copyright (c) 2006 Mike Massonnet <mmassonnet@gmail.com>
 *
 *   - xfdesktop menu plugin
 *     desktop-menu-plugin.c - xfce4-panel plugin that displays the desktop menu
 *     Copyright (C) 2004 Brian Tarricone, <bjt23@cornell.edu>
 *
 *   - launcher plugin
 *     launcher.c - (xfce4-panel plugin that opens programs)
 *     Copyright (c) 2005-2007 Jasper Huijsmans <jasper@xfce.org>
 *     Copyright (c) 2006-2007 Nick Schermer <nick@xfce.org>
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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <gtk/gtk.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfce4panel/libxfce4panel.h>

#include "button.h"
#include "util.h"

#define BOX_SPACING 2

enum
{
   PROP_0,
   PROP_LABEL,
   PROP_ELLIPSIZE
};

static void places_button_dispose(GObject * /*object*/);

static void places_button_resize(PlacesButton * /*self*/);

static void places_button_mode_changed(const XfcePanelPlugin * /*plugin*/,
                                       XfcePanelPluginMode /*mode*/,
                                       PlacesButton * /*self*/);

static gboolean places_button_size_changed(const XfcePanelPlugin * /*plugin*/, gint size, PlacesButton * /*self*/);

static void places_button_theme_changed(PlacesButton * /*self*/);

G_DEFINE_TYPE(PlacesButton, places_button, GTK_TYPE_TOGGLE_BUTTON);

void places_button_set_label(PlacesButton *self, const gchar *label)
{
   g_assert(PLACES_IS_BUTTON(self));

   if (label == NULL && self->label_text == NULL)
   {
      return;
   }

   if (label != NULL && self->label_text != NULL && strcmp(label, self->label_text) == 0)
   {
      return;
   }

   DBG("new label text: %s", label);

   if (self->label_text != NULL)
   {
      g_free(self->label_text);
   }

   self->label_text = g_strdup(label);

   places_button_resize(self);
}

void places_button_set_ellipsize(PlacesButton *self, gboolean ellipsize)
{
   g_assert(PLACES_IS_BUTTON(self));
   self->ellipsize = ellipsize;

   places_button_resize(self);
}

static void places_button_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
   PlacesButton *self = PLACES_BUTTON(object);

   switch (property_id)
   {
      case PROP_LABEL: places_button_set_label(self, g_value_get_string(value)); break;
      case PROP_ELLIPSIZE: places_button_set_ellipsize(self, g_value_get_boolean(value)); break;

      default: G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec); break;
   }
}

const gchar *places_button_get_label(PlacesButton *self)
{
   g_assert(PLACES_IS_BUTTON(self));

   DBG("returning %s", self->label_text);
   return self->label_text;
}

gboolean places_button_get_ellipsize(PlacesButton *self)
{
   g_assert(PLACES_IS_BUTTON(self));

   DBG("returning %d", self->ellipsize);
   return self->ellipsize;
}

static void places_button_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
   PlacesButton *self;

   self = PLACES_BUTTON(object);

   switch (property_id)
   {
      case PROP_LABEL: g_value_set_string(value, places_button_get_label(self)); break;
      case PROP_ELLIPSIZE: g_value_set_boolean(value, places_button_get_ellipsize(self)); break;

      default: G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec); break;
   }
}


static void places_button_class_init(PlacesButtonClass *klass)
{
   GObjectClass *gobject_class;

   gobject_class = G_OBJECT_CLASS(klass);

   gobject_class->dispose = places_button_dispose;

   gobject_class->set_property = places_button_set_property;
   gobject_class->get_property = places_button_get_property;

   g_object_class_install_property(
      gobject_class,
      PROP_LABEL,
      g_param_spec_string("label", "Label", "Button text", NULL, G_PARAM_READABLE | G_PARAM_WRITABLE));

   g_object_class_install_property(
      gobject_class,
      PROP_ELLIPSIZE,
      g_param_spec_boolean(
         "ellipsize", "Ellipsize", "Button text ellipsis", FALSE, G_PARAM_READABLE | G_PARAM_WRITABLE));
}

static void places_button_init(PlacesButton *self)
{
   self->plugin      = NULL;
   self->box         = NULL;
   self->label       = NULL;
   self->plugin_size = -1;
   self->ellipsize   = FALSE;
}

static void places_button_construct(PlacesButton *self, XfcePanelPlugin *plugin)
{
   GtkOrientation orientation;

   g_assert(XFCE_IS_PANEL_PLUGIN(plugin));

   g_object_ref(plugin);
   self->plugin = plugin;

   gtk_button_set_relief(GTK_BUTTON(self), GTK_RELIEF_NONE);
   gtk_widget_set_focus_on_click(GTK_WIDGET(self), FALSE);

   orientation = xfce_panel_plugin_get_orientation(self->plugin);
   self->box   = gtk_box_new(orientation, 1);
   gtk_container_set_border_width(GTK_CONTAINER(self->box), 0);
   gtk_widget_set_halign(self->box, GTK_ALIGN_CENTER);
   gtk_container_add(GTK_CONTAINER(self), self->box);
   gtk_widget_show(self->box);

   places_button_resize(self);

   g_signal_connect(G_OBJECT(plugin), "mode-changed", G_CALLBACK(places_button_mode_changed), self);

   g_signal_connect(G_OBJECT(plugin), "size-changed", G_CALLBACK(places_button_size_changed), self);

   self->style_set_id = g_signal_connect(G_OBJECT(self), "style-set", G_CALLBACK(places_button_theme_changed), NULL);
   self->screen_changed_id
      = g_signal_connect(G_OBJECT(self), "screen-changed", G_CALLBACK(places_button_theme_changed), NULL);
}


GtkWidget *places_button_new(XfcePanelPlugin *plugin)
{
   PlacesButton *button;

   g_assert(XFCE_IS_PANEL_PLUGIN(plugin));

   button = (PlacesButton *)g_object_new(PLACES_TYPE_BUTTON, NULL);
   places_button_construct(button, plugin);

   return GTK_WIDGET(button);
}

static void places_button_dispose(GObject *object)
{
   PlacesButton *self = PLACES_BUTTON(object);

   if (self->style_set_id != 0)
   {
      g_signal_handler_disconnect(self, self->style_set_id);
      self->style_set_id = 0;
   }

   if (self->screen_changed_id != 0)
   {
      g_signal_handler_disconnect(self, self->screen_changed_id);
      self->screen_changed_id = 0;
   }

   if (self->plugin != NULL)
   {
      g_object_unref(self->plugin);
      self->plugin = NULL;
   }

   (*G_OBJECT_CLASS(places_button_parent_class)->dispose)(object);
}

static void places_button_destroy_label(PlacesButton *self)
{
   if (self->label != NULL)
   {
      gtk_widget_destroy(self->label);
      g_object_unref(self->label);
      self->label = NULL;
   }
}

static void places_button_resize_label(PlacesButton *self, unused gboolean show)
{
   gboolean vertical = FALSE;
   gboolean deskbar  = FALSE;

   if (xfce_panel_plugin_get_mode(self->plugin) == XFCE_PANEL_PLUGIN_MODE_DESKBAR)
   {
      deskbar = TRUE;
   }
   else if (xfce_panel_plugin_get_mode(self->plugin) == XFCE_PANEL_PLUGIN_MODE_VERTICAL)
   {
      vertical = TRUE;
   }
   if (self->label_text == NULL)
   {
      places_button_destroy_label(self);
      return;
   }

   if (self->label == NULL)
   {
      self->label = g_object_ref(gtk_label_new(self->label_text));
      gtk_box_pack_end(GTK_BOX(self->box), self->label, TRUE, TRUE, 0);
   }
   else
   {
      gtk_label_set_text(GTK_LABEL(self->label), self->label_text);
   }

   if (deskbar || self->ellipsize)
   {
      gtk_label_set_ellipsize(GTK_LABEL(self->label), PANGO_ELLIPSIZE_END);
   }
   else
   {
      gtk_label_set_ellipsize(GTK_LABEL(self->label), PANGO_ELLIPSIZE_NONE);
   }

   if (vertical)
   {
      gtk_label_set_angle(GTK_LABEL(self->label), -RIGTH_ANGLE);
      gtk_widget_set_halign(self->label, GTK_ALIGN_CENTER);
      gtk_widget_set_valign(self->label, GTK_ALIGN_START);
   }
   else
   {
      gtk_label_set_angle(GTK_LABEL(self->label), 0);
      gtk_widget_set_halign(self->label, GTK_ALIGN_START);
      gtk_widget_set_valign(self->label, GTK_ALIGN_CENTER);
   }
   if (self->ellipsize)
   {
      gtk_label_set_max_width_chars(GTK_LABEL(self->label), MIN_FACT_LEN);
   }
   else
   {
      gtk_label_set_max_width_chars(GTK_LABEL(self->label), MAX_FACT_LEN);
   }

   gtk_widget_show(self->label);
}


static void places_button_resize(PlacesButton *self)
{
   gboolean show_label;
   gint     new_size;
   gboolean vertical = FALSE;
   gboolean deskbar  = FALSE;

   if (self->plugin == NULL)
   {
      return;
   }

   new_size          = xfce_panel_plugin_get_size(self->plugin);
   self->plugin_size = new_size;
   DBG("Panel size: %d", new_size);

   show_label = self->label_text != NULL;

   if (xfce_panel_plugin_get_mode(self->plugin) == XFCE_PANEL_PLUGIN_MODE_DESKBAR)
   {
      deskbar = TRUE;
   }
   else if (xfce_panel_plugin_get_mode(self->plugin) == XFCE_PANEL_PLUGIN_MODE_VERTICAL)
   {
      vertical = TRUE;
   }

   if (show_label)
   {
      xfce_panel_plugin_set_small(self->plugin, deskbar ? FALSE : TRUE);
      if (vertical)
      {
         gtk_widget_set_halign(self->box, GTK_ALIGN_CENTER);
         gtk_widget_set_valign(self->box, GTK_ALIGN_START);
      }
      else
      {
         gtk_widget_set_halign(self->box, GTK_ALIGN_START);
         gtk_widget_set_valign(self->box, GTK_ALIGN_CENTER);
      }
   }
   else
   {
      xfce_panel_plugin_set_small(self->plugin, TRUE);
      gtk_widget_set_halign(self->box, GTK_ALIGN_CENTER);
      gtk_widget_set_valign(self->box, GTK_ALIGN_CENTER);
   }

   /* label */
   places_button_resize_label(self, show_label);
}

static void places_button_mode_changed(unused const XfcePanelPlugin *plugin,
                                       XfcePanelPluginMode           mode,
                                       PlacesButton                 *self)
{
   DBG("orientation changed");
   gtk_orientable_set_orientation(
      GTK_ORIENTABLE(self->box),
      (mode == XFCE_PANEL_PLUGIN_MODE_VERTICAL) ? GTK_ORIENTATION_VERTICAL : GTK_ORIENTATION_HORIZONTAL);
   places_button_resize(self);
}

static gboolean places_button_size_changed(unused const XfcePanelPlugin *plugin, gint size, PlacesButton *self)
{
   if (self->plugin_size == size)
   {
      return TRUE;
   }

   DBG("size changed");
   places_button_resize(self);
   return TRUE;
}

static void places_button_theme_changed(PlacesButton *self)
{
   DBG("theme changed");
   places_button_resize(self);
}

/* vim: set ai et tabstop=4: */
