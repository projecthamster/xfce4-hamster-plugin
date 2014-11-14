/*  xfce4-hamster-plugin
 *
 *  Copyright (c) 2014 Hakan Erduman <smultimeter@gmail.com>
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include <libxfce4ui/libxfce4ui.h>
#include <libxfce4panel/libxfce4panel.h>
#include <xfconf/xfconf.h>
#include "settings.h"

void
config_show(XfcePanelPlugin *plugin, XfconfChannel *channel)
{
   GtkWidget *dlg = xfce_titled_dialog_new();
   GtkWidget *cnt, *lbl, *chk;
   g_object_set(G_OBJECT(dlg),
         "title", _("Hamster"),
         "icon_name", ICON_NAME,
         "subtitle", _("Time bookkeeping plugin"),
         NULL);
   g_signal_connect_swapped (dlg,
                             "response",
                             G_CALLBACK (gtk_widget_destroy),
                             dlg);

   cnt = gtk_dialog_get_content_area(GTK_DIALOG(dlg));

   lbl = gtk_label_new(_("<b>Settings</b>"));
   gtk_label_set_use_markup(GTK_LABEL(lbl), TRUE);
   gtk_misc_set_alignment(GTK_MISC(lbl), 0.1, 0.5);
   gtk_container_add(GTK_CONTAINER(cnt), lbl);

   chk = gtk_check_button_new_with_label(_("Keep popup floating"));
   xfconf_g_property_bind(channel, XFPROP_DONTHIDE, G_TYPE_BOOLEAN, G_OBJECT(chk), "active");
   gtk_container_add(GTK_CONTAINER(cnt), chk);

   chk = gtk_check_button_new_with_label(_("Entry completion as dropdown"));
   xfconf_g_property_bind(channel, XFPROP_DROPDOWN, G_TYPE_BOOLEAN, G_OBJECT(chk), "active");
   gtk_container_add(GTK_CONTAINER(cnt), chk);

   chk = gtk_check_button_new_with_label(_("Show tooltips on buttons"));
   xfconf_g_property_bind(channel, XFPROP_TOOLTIPS, G_TYPE_BOOLEAN, G_OBJECT(chk), "active");
   gtk_container_add(GTK_CONTAINER(cnt), chk);

   gtk_dialog_add_button(GTK_DIALOG(dlg), GTK_STOCK_CLOSE, 0);

   gtk_widget_show_all(dlg);
   gtk_dialog_run(GTK_DIALOG(dlg));
   gtk_widget_destroy(dlg);
}
