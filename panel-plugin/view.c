#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
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
    GtkWidget                 *popup;
    GtkWidget                 *entry;
    GtkWidget                 *summary;

    /* model */
    GtkListStore              *storeFacts;
    GtkListStore              *storeActivities;
    Hamster                   *hamster;
};

/* this ugly global variable is to bypass a bug in the 'match-select' signal;
 * user_data is no passed */
HamsterView *g_view;

enum
{
   TIME_SPAN,
   TITLE,
   DURATION,
   BTNEDIT,
   BTNCONT,
   ID,
   NUM_COL
};

/* Button */
static void
hview_popup_hide(HamsterView *view)
{
    /* untoggle the button */
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(view->button), FALSE);

    /* empty entry */
    if(view->entry)
    {
       gtk_entry_set_text(GTK_ENTRY(view->entry), "");
    }

    /* hide the dialog for reuse */
    if (view->popup)
    {
       gtk_widget_hide (view->popup);
    }
}

/* Menu callbacks */
static void
hview_cb_show_overview(HamsterView *view)
{
   g_spawn_command_line_sync("hamster-time-tracker overview", NULL, NULL, NULL, NULL);
}

static void
hview_cb_stop_tracking(HamsterView *view)
{
   g_spawn_command_line_sync("hamster-cli stop", NULL, NULL, NULL, NULL);
}

static void
hview_cb_add_earlier_activity(HamsterView *view)
{
   g_spawn_command_line_sync("hamster-time-tracker edit", NULL, NULL, NULL, NULL);
}

static void
hview_cb_tracking_settings(HamsterView *view)
{
   g_spawn_command_line_sync("hamster-time-tracker preferences", NULL, NULL, NULL, NULL);
}

gboolean
hview_cb_popup_focus_out (GtkWidget *widget,
                    GdkEventFocus *event,
                    HamsterView *view)
{
   hview_popup_hide(view);
   return TRUE;
}

static gboolean
hview_cb_match_select(GtkEntryCompletion *widget,
                     GtkTreeModel *model,
                     GtkTreeIter *iter,
                     gpointer userdata)
{
   gchar *activity, *category;
   gchar fact[256];
   gint id = 0;
   GtkEntry *entry;

   gtk_tree_model_get(model, iter, 0, &activity, 1, &category, -1);
   sprintf(fact, "%s@%s", activity, category);
   hamster_call_add_fact_sync(g_view->hamster, fact, 0, 0, FALSE, &id, NULL, NULL);
   g_free(activity);
   g_free(category);
   DBG("activated: %s[%d]", fact, id);
   hview_popup_hide(g_view);
   return FALSE;
}

void
hview_cb_entry_activate(GtkEntry *entry,
                  HamsterView *view)
{
   const gchar *fact = gtk_entry_get_text(GTK_ENTRY(g_view->entry));
   gint id = 0;
   hamster_call_add_fact_sync(g_view->hamster, fact, 0, 0, FALSE, &id, NULL, NULL);
   DBG("activated: %s[%d]", fact, id);
   hview_popup_hide(g_view);
}

static void
hview_popup_new(HamsterView *view)
{
   GtkWidget *gui, *vbx, *lbl, *ovw, *stp, *add, *sep, *cfg, *menu;
   GtkCellRenderer *renderer;
   GtkTreeViewColumn *column;
   GtkEntryCompletion *completion;

   /* Create a new popup */
   view->popup = gtk_window_new(GTK_WINDOW_TOPLEVEL);
   gtk_window_set_decorated(GTK_WINDOW(view->popup), FALSE);
   gtk_window_set_position(GTK_WINDOW(view->popup), GTK_WIN_POS_MOUSE);
   gtk_window_set_screen(GTK_WINDOW(view->popup),
                       gtk_widget_get_screen(view->button));
   gtk_window_set_skip_pager_hint(GTK_WINDOW(view->popup), TRUE);
   gtk_window_set_skip_taskbar_hint(GTK_WINDOW(view->popup), TRUE);
   gtk_container_set_border_width (GTK_CONTAINER (view->popup), 10);
   vbx = gtk_vbox_new(FALSE, 1);
   gtk_container_add(GTK_CONTAINER(view->popup), vbx);

   // subtitle
   lbl = gtk_label_new(_("What goes on?"));
   gtk_container_add(GTK_CONTAINER(vbx), lbl);

   // entry
   view->entry = gtk_entry_new();
   completion = gtk_entry_completion_new();
   g_signal_connect_swapped(completion, "match-selected",
                           G_CALLBACK(hview_cb_match_select), NULL);
   g_signal_connect_swapped(view->entry, "activate",
                           G_CALLBACK(hview_cb_entry_activate), view);
   gtk_entry_completion_set_text_column(completion, 0);
   gtk_entry_completion_set_model(completion, GTK_TREE_MODEL(view->storeActivities));
   gtk_container_add(GTK_CONTAINER(vbx), view->entry);
   gtk_entry_set_completion(GTK_ENTRY(view->entry), completion);

   // label
   lbl = gtk_label_new(_("Todays activities"));
   gtk_container_add(GTK_CONTAINER(vbx), lbl);

   // todays activities
   lbl = gtk_tree_view_new_with_model(GTK_TREE_MODEL(view->storeFacts));
   gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(lbl), FALSE);
   gtk_tree_view_set_hover_selection(GTK_TREE_VIEW(lbl), TRUE);
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

   gtk_misc_set_alignment(GTK_MISC(view->summary), 1.0, 1.0);
   gtk_container_add(GTK_CONTAINER(vbx), view->summary);

   ovw = gtk_menu_item_new_with_label(_("Show overview"));
   g_signal_connect_swapped(ovw, "activate",
                           G_CALLBACK(hview_cb_show_overview), view);
   stp = gtk_menu_item_new_with_label(_("Stop tracking"));
   g_signal_connect_swapped(stp, "activate",
                           G_CALLBACK(hview_cb_stop_tracking), view);
   add = gtk_menu_item_new_with_label(_("Add earlier activity"));
   g_signal_connect_swapped(add, "activate",
                           G_CALLBACK(hview_cb_add_earlier_activity), view);
   sep = gtk_separator_menu_item_new();
   cfg = gtk_menu_item_new_with_label(_("Tracking settings"));
   g_signal_connect_swapped(cfg, "activate",
                           G_CALLBACK(hview_cb_tracking_settings), view);

   DBG("TODO: menus have single click");
   menu = gtk_menu_bar_new();
   gtk_menu_bar_set_pack_direction(GTK_MENU_BAR(menu), GTK_PACK_DIRECTION_TTB);
   gtk_container_add(GTK_CONTAINER(vbx), menu);
   gtk_menu_shell_append(GTK_MENU_SHELL(menu), gui);
   gtk_menu_shell_append(GTK_MENU_SHELL(menu), ovw);
   gtk_menu_shell_append(GTK_MENU_SHELL(menu), stp);
   gtk_menu_shell_append(GTK_MENU_SHELL(menu), add);
   gtk_menu_shell_append(GTK_MENU_SHELL(menu), sep);
   gtk_menu_shell_append(GTK_MENU_SHELL(menu), cfg);

   gtk_widget_show_all(view->popup);

   g_signal_connect (G_OBJECT (view->popup),
                    "focus-out-event",
                    G_CALLBACK (hview_cb_popup_focus_out),
                    view);
}

/* Actions */
static void
hview_popup_show(HamsterView *view)
{
   /* toggle the button */
   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(view->button), TRUE);

   /* check if popup is needed, or it needs an update */
   if(view->popup == NULL)
       hview_popup_new(view);

   /* popup popup */
   gtk_window_present_with_time(GTK_WINDOW(view->popup), gtk_get_current_event_time ());
   gtk_widget_add_events (view->popup, GDK_FOCUS_CHANGE_MASK);
}

static void
hview_store_update(HamsterView *view, fact *act, GHashTable *tbl)
{

   if(NULL == view->storeFacts)
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

      gtk_list_store_append (view->storeFacts, &iter);  /* Acquire an iterator */

      gtk_list_store_set (view->storeFacts, &iter,
                          TIME_SPAN, spn,
                          TITLE, act->name,
                          DURATION, dur,
                          BTNEDIT, "gtk-edit",
                          BTNCONT, "gtk-media-play",
                          /*ID, act->id,*/
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
hview_completion_update(HamsterView *view)
{
   GVariant *res;
   if(NULL != view->storeActivities)
      gtk_list_store_clear(view->storeActivities);
   if(NULL != view->hamster)
   {
      if(hamster_call_get_activities_sync(view->hamster, "", &res, NULL, NULL))
      {
         gsize count = 0;
         if(NULL != res && (count = g_variant_n_children(res)))
         {
            int i;
            for(i=0; i< count; i++)
            {
               GtkTreeIter iter;
               GVariant *dbusAct = g_variant_get_child_value(res, i);
               gchar *act, *cat;
               g_variant_get(dbusAct, "(ss)", &act, &cat);
               gtk_list_store_append(view->storeActivities, &iter);
               gtk_list_store_set(view->storeActivities, &iter,
                     0, act, 1, cat, -1);
            }
         }
      }
   }
}

static void
hview_button_update(HamsterView *view)
{
   GVariant *res;
   if(NULL != view->storeFacts)
      gtk_list_store_clear(view->storeFacts);
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
   /* (it's the way xfdesktop popup does it...) */
   if ((evt->state & GDK_CONTROL_MASK)
         && !(evt->state & (GDK_MOD1_MASK | GDK_SHIFT_MASK | GDK_MOD4_MASK)))
      return FALSE;

   if (evt->button == 1)
   {
      gboolean isActive = gtk_toggle_button_get_active(
            GTK_TOGGLE_BUTTON(view->button));
      if (isActive)
         hview_popup_hide(view);
      else
         hview_popup_show(view);
   }
   else if (evt->button == 2)
   {
      hview_cb_show_overview(view);
   }
   hview_button_update(view);
   return TRUE;
}

static gboolean
hview_cb_hamster_changed(HamsterView *view)
{
   hview_button_update(view);
   hview_completion_update(view);
   return FALSE;
}

static gboolean
hview_cb_cyclic(HamsterView *view)
{
   hview_button_update(view);
   return TRUE;
}

HamsterView*
hamster_view_init(XfcePanelPlugin* plugin)
{
   DBG("initializing");
   g_assert(plugin != NULL);

   g_view            = g_new0(HamsterView, 1);
   g_view->plugin    = plugin;

   /* init button */

   DBG("init GUI");

   /* create the button */
   g_view->button = g_object_ref(places_button_new(g_view->plugin));
   xfce_panel_plugin_add_action_widget(g_view->plugin, g_view->button);
   gtk_container_add(GTK_CONTAINER(g_view->plugin), g_view->button);
   gtk_widget_show(g_view->button);

   /* signals for icon theme/screen changes */
   g_signal_connect_swapped(g_view->button, "style-set",
                            G_CALLBACK(hview_popup_hide), g_view);
   g_signal_connect_swapped(g_view->button, "screen-changed",
                            G_CALLBACK(hview_popup_hide), g_view);

   /* button signal */
   g_signal_connect_swapped(g_view->button, "button-press-event",
                            G_CALLBACK(hview_cb_button_pressed), g_view);

   g_timeout_add_seconds(60, (GSourceFunc)hview_cb_cyclic, g_view);

   /* remote control */
   g_view->hamster = hamster_proxy_new_for_bus_sync
         (
                     G_BUS_TYPE_SESSION,
                     G_DBUS_PROXY_FLAGS_NONE,
                     "org.gnome.Hamster",              /* bus name */
                     "/org/gnome/Hamster", /* object */
                     NULL,                          /* GCancellable* */
                     NULL);

   g_signal_connect_swapped(g_view->hamster, "facts-changed",
                            G_CALLBACK(hview_cb_hamster_changed), g_view);
   g_signal_connect_swapped(g_view->hamster, "activities-changed",
                            G_CALLBACK(hview_cb_hamster_changed), g_view);

   /* storage */
   g_view->storeActivities = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
   g_view->storeFacts = gtk_list_store_new(NUM_COL, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
         G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT);
   g_view->summary = gtk_label_new(_("No activities yet."));

   /* liftoff */
   hview_button_update(g_view);
   hview_completion_update(g_view);

   DBG("done");

   return g_view;
}

void
hamster_view_finalize(HamsterView* view)
{
   g_free(view);
}


