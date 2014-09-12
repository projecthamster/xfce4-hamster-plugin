#include <libxfce4util/libxfce4util.h>
#include <libxfce4panel/libxfce4panel.h>
#include "view.h"
#include "button.h"
#include "hamster.h"
#include "util.h"
#include <gdk/gdk.h>

struct _HamsterView
{
    /* plugin */
    XfcePanelPlugin           *plugin;

    /* view */
    GtkWidget                 *button;
    GtkWidget                 *menu;
    GtkWidget                 *summary;

    /* model */
    GtkListStore              *store;
    Hamster                   *hamster;
};

enum
{
   TIME_SPAN,
   TITLE,
   DURATION,
   BTNEDIT,
   BTNCONT,
   NUM_COL
};

/* Button */
static void
hview_destroy_dialog(HamsterView *view)
{
    /* untoggle the button */
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(view->button), FALSE);
}

/* Menu callbacks */
static void
hview_show_overview(HamsterView *view)
{
   g_spawn_command_line_sync("hamster-time-tracker overview", NULL, NULL, NULL, NULL);
}

static void
hview_stop_tracking(HamsterView *view)
{
   g_spawn_command_line_sync("hamster-cli stop", NULL, NULL, NULL, NULL);
}

static void
hview_add_earlier_activity(HamsterView *view)
{
   g_spawn_command_line_sync("hamster-time-tracker edit", NULL, NULL, NULL, NULL);
}

static void
hview_tracking_settings(HamsterView *view)
{
   g_spawn_command_line_sync("hamster-time-tracker preferences", NULL, NULL, NULL, NULL);
}

static void
hview_menu_new(HamsterView *view)
{
   GtkWidget *gui, *vbx, *lbl, *ovw, *stp, *add, *sep, *cfg;
   GtkCellRenderer *renderer;
   GtkTreeViewColumn *column;
   /* Create a new menu */
   view->menu = gtk_menu_new();

   /* make sure the menu popups up in right screen */
   gtk_menu_attach_to_widget(GTK_MENU(view->menu), view->button, NULL);
   gtk_menu_set_screen(GTK_MENU(view->menu),
                       gtk_widget_get_screen(view->button));
   g_signal_connect_swapped(view->menu, "selection-done",
                            G_CALLBACK(hview_destroy_dialog), view);


   gui = gtk_menu_item_new();
   vbx = gtk_vbox_new(FALSE, 1);
   gtk_container_add(GTK_CONTAINER(gui), vbx);
   lbl = gtk_label_new(_("What are you doing?"));
   gtk_container_add(GTK_CONTAINER(vbx), lbl);
   lbl = gtk_entry_new();
   gtk_container_add(GTK_CONTAINER(vbx), lbl);
   lbl = gtk_label_new(_("Todays activities"));
   gtk_container_add(GTK_CONTAINER(vbx), lbl);

   view->store = gtk_list_store_new(NUM_COL, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
         G_TYPE_STRING, G_TYPE_STRING);
   lbl = gtk_tree_view_new_with_model(GTK_TREE_MODEL(view->store));
   gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(lbl), FALSE);
   renderer = gtk_cell_renderer_text_new ();
   column = gtk_tree_view_column_new_with_attributes ("Time",
                                                      renderer,
                                                      "text", TIME_SPAN,
                                                      NULL);
   gtk_tree_view_append_column (GTK_TREE_VIEW (lbl), column);
   column = gtk_tree_view_column_new_with_attributes ("Name",
                                                      renderer,
                                                      "text", TITLE,
                                                      NULL);
   gtk_tree_view_append_column (GTK_TREE_VIEW (lbl), column);
   column = gtk_tree_view_column_new_with_attributes ("Duration",
                                                      renderer,
                                                      "text", DURATION,
                                                      NULL);
   gtk_tree_view_append_column (GTK_TREE_VIEW (lbl), column);
   renderer = gtk_cell_renderer_pixbuf_new();
   column = gtk_tree_view_column_new_with_attributes ("ed",
                                                      renderer,
                                                      "stock-id", BTNEDIT,
                                                      NULL);
   gtk_tree_view_append_column (GTK_TREE_VIEW (lbl), column);
   column = gtk_tree_view_column_new_with_attributes ("ct",
                                                      renderer,
                                                      "stock-id", BTNCONT,
                                                      NULL);
   gtk_tree_view_append_column (GTK_TREE_VIEW (lbl), column);
   gtk_container_add(GTK_CONTAINER(vbx), lbl);

   view->summary = gtk_label_new("...");
   gtk_misc_set_alignment(GTK_MISC(view->summary), 1.0, 1.0);
   gtk_container_add(GTK_CONTAINER(vbx), view->summary);

   ovw = gtk_menu_item_new_with_label(_("Show overview"));
   g_signal_connect_swapped(ovw, "activate",
                           G_CALLBACK(hview_show_overview), view);
   stp = gtk_menu_item_new_with_label(_("Stop tracking"));
   g_signal_connect_swapped(stp, "activate",
                           G_CALLBACK(hview_stop_tracking), view);
   add = gtk_menu_item_new_with_label(_("Add earlier activity"));
   g_signal_connect_swapped(add, "activate",
                           G_CALLBACK(hview_add_earlier_activity), view);
   sep = gtk_separator_menu_item_new();
   cfg = gtk_menu_item_new_with_label(_("Tracking settings"));
   g_signal_connect_swapped(cfg, "activate",
                           G_CALLBACK(hview_tracking_settings), view);

   gtk_menu_shell_append(GTK_MENU_SHELL(view->menu), gui);
   gtk_menu_shell_append(GTK_MENU_SHELL(view->menu), ovw);
   gtk_menu_shell_append(GTK_MENU_SHELL(view->menu), stp);
   gtk_menu_shell_append(GTK_MENU_SHELL(view->menu), add);
   gtk_menu_shell_append(GTK_MENU_SHELL(view->menu), sep);
   gtk_menu_shell_append(GTK_MENU_SHELL(view->menu), cfg);
   gtk_widget_show_all(view->menu);

}

/* Actions */
static void
hview_open_menu(HamsterView *view)
{
   /* check if menu is needed, or it needs an update */
   if(view->menu == NULL)
       hview_menu_new(view);

   /* toggle the button */
   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(view->button), TRUE);

   /* popup menu */
   DBG("menu: %x", (guint)view->menu);
   gtk_menu_popup (GTK_MENU (view->menu), NULL, NULL,
                   (view->button != NULL) ? xfce_panel_plugin_position_menu : NULL,
                   view->plugin, 1,
                   gtk_get_current_event_time ());

}

static void
hview_store_update(HamsterView *view, fact *act, GHashTable *tbl)
{

   if(NULL == view->store)
      return;
   else
   {
      GtkTreeIter   iter;
      gchar spn[20];
      gchar dur[20];
      gchar *ptr;
      gint *val;

      struct tm *tma = gmtime(&act->startTime);
      strftime(spn, 20, "%H:%M", tma);
      strcat(spn, " - ");
      ptr = spn + strlen(spn);
      if(act->endTime)
      {
         tma = gmtime(&act->endTime);
         strftime(ptr, 20, "%H:%M", tma);
      }
      sprintf(dur, "%dh %dmin", act->seconds / 3600, (act->seconds / 60) % 60);

      gtk_list_store_append (view->store, &iter);  /* Acquire an iterator */

      gtk_list_store_set (view->store, &iter,
                          TIME_SPAN, spn,
                          TITLE, act->name,
                          DURATION, dur,
                          BTNEDIT, "gtk-edit",
                          BTNCONT, "gtk-media-play",
                          -1);

      val = g_hash_table_lookup(tbl, act->category);
      if(NULL == val)
      {
         val = g_new0(gint, 1);
         g_hash_table_insert(tbl, strdup(act->category), val);
      }
      *val += act->seconds;
   }
}

static void
hview_summary_update(HamsterView *view, GHashTable *tbl)
{
   GHashTableIter iter;
   GString *string = g_string_new("");
   gchar *cat;
   gint *sum;
   guint count;

   count = g_hash_table_size(tbl);
   g_hash_table_iter_init(&iter, tbl);
   while(g_hash_table_iter_next(&iter, (gpointer)&cat, (gpointer)&sum))
   {
      count--;
      g_string_append_printf(string, count ? "%s: %d.%1d, " : "%s: %d.%1d ",
            cat, *sum / 3600, (10 * (*sum % 3600)) / 3600);
   }
   gtk_label_set_label(GTK_LABEL(view->summary), string->str);
   g_string_free(string, TRUE);
}

static void
hview_button_update(HamsterView *view)
{
   GVariant *res;
   if(NULL != view->store)
      gtk_list_store_clear(view->store);
   if(NULL != view->hamster)
   {
      if(hamster_call_get_todays_facts_sync(view->hamster, &res, NULL, NULL))
      {
         gsize count = 0;
         if(NULL != res && (count = g_variant_n_children(res)))
         {
            int i;
            GHashTable *tbl = g_hash_table_new(g_str_hash, g_str_equal);
            for(i = 0; i < count; i++)
            {
               GVariant *dbusFact = g_variant_get_child_value(res, i);
               fact *last = fact_new(dbusFact);
               g_variant_unref(dbusFact);
               hview_store_update(view, last, tbl);
               if(last->id && i == count -1)
               {
                  hview_summary_update(view, tbl);
                  if(0 == last->endTime)
                  {
                     gchar label[128];
                     sprintf(label, "%s %d:%02d", last->name, last->seconds / 3600, (last->seconds/60) % 60);
                     places_button_set_label(PLACES_BUTTON(view->button), label);
                     fact_free(last);
                     g_hash_table_unref(tbl);
                     return;
                  }
               }
               fact_free(last);
            }
            g_hash_table_unref(tbl);
         }
      }
   }
   places_button_set_label(PLACES_BUTTON(view->button), _("inactive"));
}

static gboolean
hview_cb_button_pressed(HamsterView *view, GdkEventButton *evt)
{
    /* (it's the way xfdesktop menu does it...) */
    if((evt->state & GDK_CONTROL_MASK) && !(evt->state & (GDK_MOD1_MASK|GDK_SHIFT_MASK|GDK_MOD4_MASK)))
        return FALSE;

    if(evt->button == 1)
        hview_open_menu(view);
    else if(evt->button == 2)
        hview_show_overview(view);
    hview_button_update(view);
    return FALSE;
}

static gboolean
hview_cb_hamster_changed(HamsterView *view)
{
   hview_button_update(view);
   return FALSE;
}

static gboolean
hview_cyclic(HamsterView *view)
{
   hview_button_update(view);
   return TRUE;
}

HamsterView*
hamster_view_init(XfcePanelPlugin* plugin)
{
   HamsterView *view;                   /* internal use in this file */

   DBG("initializing");
   g_assert(plugin != NULL);

   view            = g_new0(HamsterView, 1);
   view->plugin    = plugin;

   /* init button */

   DBG("init GUI");

   /* create the button */
   view->button = g_object_ref(places_button_new(view->plugin));
   xfce_panel_plugin_add_action_widget(view->plugin, view->button);
   gtk_container_add(GTK_CONTAINER(view->plugin), view->button);
   gtk_widget_show(view->button);

   /* signals for icon theme/screen changes */
   g_signal_connect_swapped(view->button, "style-set",
                            G_CALLBACK(hview_destroy_dialog), view);
   g_signal_connect_swapped(view->button, "screen-changed",
                            G_CALLBACK(hview_destroy_dialog), view);

   /* button signal */
   g_signal_connect_swapped(view->button, "button-press-event",
                            G_CALLBACK(hview_cb_button_pressed), view);

   g_timeout_add_seconds(60, (GSourceFunc)hview_cyclic, view);

   /* remote control */
   view->hamster = hamster_proxy_new_for_bus_sync
         (
                     G_BUS_TYPE_SESSION,
                     G_DBUS_PROXY_FLAGS_NONE,
                     "org.gnome.Hamster",              /* bus name */
                     "/org/gnome/Hamster", /* object */
                     NULL,                          /* GCancellable* */
                     NULL);

   g_signal_connect_swapped(view->hamster, "facts-changed",
                            G_CALLBACK(hview_cb_hamster_changed), view);
   g_signal_connect_swapped(view->hamster, "activities-changed",
                            G_CALLBACK(hview_cb_hamster_changed), view);
   /* liftoff */
   hview_button_update(view);

   DBG("done");

   return view;
}

void
hamster_view_finalize(HamsterView* view)
{
   g_free(view);
}


