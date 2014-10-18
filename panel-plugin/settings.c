#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include <libxfcegui4/libxfcegui4.h>
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
