#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfce4panel/libxfce4panel.h>
#include <xfconf/xfconf.h>
#include "view.h"
#include "button.h"
#include "hamster.h"
#include "windowserver.h"
#include "util.h"
#include "settings.h"

struct _HamsterView
{
    /* plugin */
    XfcePanelPlugin           *plugin;

    /* view */
    GtkWidget                 *button;
    GtkWidget                 *popup;
    GtkWidget                 *entry;
    GtkWidget                 *treeview;
    GtkWidget                 *summary;

    /* model */
    GtkListStore              *storeFacts;
    GtkListStore              *storeActivities;
    Hamster                   *hamster;
    WindowServer              *windowserver;

    /* config */
    XfconfChannel             *channel;
    gboolean                  donthide;
    gboolean                  tooltips;
};

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
       gtk_widget_grab_focus(view->entry);
    }

    /* hide the dialog for reuse */
    if (view->popup)
    {
       gtk_widget_hide (view->popup);
    }
}

/* Button callbacks */
static void
hview_cb_show_overview(GtkWidget *widget, HamsterView *view)
{
   window_server_call_overview_sync(view->windowserver, NULL, NULL);
   hview_popup_hide(view);
}

static void
hview_cb_stop_tracking(GtkWidget *widget, HamsterView *view)
{
   time_t now = time(NULL);
   GVariant *stopTime = g_variant_new_int32(now - timezone + (daylight * 3600));
   GVariant *var = g_variant_new_variant(stopTime);
   hamster_call_stop_tracking_sync(view->hamster, var, NULL, NULL);
   hview_popup_hide(view);
}

static void
hview_cb_add_earlier_activity(GtkWidget *widget, HamsterView *view)
{
   /* pass empty variants?
   GVariant *dummy = g_variant_new_maybe(G_VARIANT_TYPE_VARIANT, NULL);
   GVariant *var = g_variant_new_variant(dummy);
   window_server_call_edit_sync(view->windowserver, var, NULL, NULL);
   */
   {
     GVariant *_ret;
     _ret = g_dbus_proxy_call_sync (G_DBUS_PROXY (view->windowserver),
       "edit",
       g_variant_new ("()"),
       G_DBUS_CALL_FLAGS_NONE,
       -1,
       NULL,
       NULL);
     if (_ret == NULL)
       goto _out;
     g_variant_get (_ret,
                    "()");
     g_variant_unref (_ret);
   }
   _out:
   hview_popup_hide(view);
}

static void
hview_cb_tracking_settings(GtkWidget *widget, HamsterView *view)
{
   window_server_call_preferences_sync(view->windowserver, NULL, NULL);
   hview_popup_hide(view);
}

gboolean
hview_cb_popup_focus_out (GtkWidget *widget,
                    GdkEventFocus *event,
                    HamsterView *view)
{
   if(view->donthide)
      return FALSE;
   hview_popup_hide(view);
   return TRUE;
}

static gboolean
hview_cb_match_select(GtkEntryCompletion *widget,
                     GtkTreeModel *model,
                     GtkTreeIter *iter,
                     HamsterView *view)
{
   gchar *activity, *category;
   gchar fact[256];
   gint id = 0;
   GtkEntry *entry;

   gtk_tree_model_get(model, iter, 0, &activity, 1, &category, -1);
   sprintf(fact, "%s@%s", activity, category);
   hamster_call_add_fact_sync(view->hamster, fact, 0, 0, FALSE, &id, NULL, NULL);
   g_free(activity);
   g_free(category);
   DBG("activated: %s[%d]", fact, id);
   hview_popup_hide(view);
   return FALSE;
}

static void
hview_cb_entry_activate(GtkEntry *entry,
                  HamsterView *view)
{
   const gchar *fact = gtk_entry_get_text(GTK_ENTRY(view->entry));
   gint id = 0;
   hamster_call_add_fact_sync(view->hamster, fact, 0, 0, FALSE, &id, NULL, NULL);
   DBG("activated: %s[%d]", fact, id);
   hview_popup_hide(view);
}

static gboolean
hview_cb_tv_query_tooltip(GtkWidget  *widget,
               gint        x,
               gint        y,
               gboolean    keyboard_mode,
               GtkTooltip *tooltip,
               HamsterView *view)
{
   if(view->tooltips)
   {
      GtkTreePath *path;
      GtkTreeViewColumn *column;
      if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(widget), x, y, &path,
            &column, NULL, NULL))
      {
         gchar *text = g_object_get_data(G_OBJECT(column), "tip");
         if (text)
         {
            gtk_tooltip_set_text(tooltip, text);
            return TRUE;
         }
      }
   }
   return FALSE;
}

static gboolean
hview_cb_tv_button_press(GtkWidget *tv,
                  GdkEventButton* evt,
                  HamsterView *view)
{
   if (evt->button == 1)
   {
      GtkTreePath *path;
      GtkTreeViewColumn *column;
      if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(tv), evt->x,
            evt->y, &path, &column, NULL, NULL))
      {
         GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tv));
         GtkTreeModel *model = NULL;
         GtkTreeIter iter;
         if (gtk_tree_selection_get_selected(selection, &model, &iter))
         {
            gint id;
            gchar *icon;
            gchar *fact;
            gtk_tree_model_get(model, &iter, ID, &id, BTNCONT, &icon, TITLE, &fact, -1);
            DBG("%s:%d:%s", column->title, id, icon);
            if(!strcmp(column->title, "ed"))
            {
               GVariant *dummy = g_variant_new_int32(id);
               GVariant *var = g_variant_new_variant(dummy);
                  window_server_call_edit_sync(view->windowserver, var, NULL, NULL);
            }
            else if(!strcmp(column->title, "ct") && !strcmp(icon, "gtk-media-play"))
            {
               DBG("Resume %s", fact);
               hamster_call_add_fact_sync(view->hamster, fact, 0, 0, FALSE, &id, NULL, NULL);
            }
            g_free(icon);
            g_free(fact);
         }
         gtk_tree_path_free(path);
      }
   }
   return FALSE;
}

static void
hview_cb_label_allocate( GtkWidget *label,
             GtkAllocation *allocation,
             HamsterView *view )
{
   GtkRequisition req;
   gtk_widget_size_request(view->treeview, &req);
   gtk_widget_set_size_request( label, req.width, -1 );
}

static gboolean
hview_cb_key_pressed(GtkWidget *widget,
      GdkEventKey  *evt,
      HamsterView *view)
{
   //DBG("%d", evt->keyval);
   if(evt->keyval == GDK_KEY_Escape)
   {
      hview_popup_hide(view);
   }
   return FALSE;
}

static void
hview_popup_new(HamsterView *view)
{
   GtkWidget *frm, *vbx, *lbl, *ovw, *stp, *add, *sep, *cfg, *align;
   GtkCellRenderer *renderer;
   GtkTreeViewColumn *column;
   GtkEntryCompletion *completion;
   gint border;

   /* Create a new popup */
   view->popup = gtk_window_new(GTK_WINDOW_TOPLEVEL);
   gtk_window_set_decorated(GTK_WINDOW(view->popup), FALSE);
   gtk_window_set_position(GTK_WINDOW(view->popup), GTK_WIN_POS_MOUSE);
   gtk_window_set_screen(GTK_WINDOW(view->popup),
                       gtk_widget_get_screen(view->button));
   gtk_window_set_skip_pager_hint(GTK_WINDOW(view->popup), TRUE);
   gtk_window_set_skip_taskbar_hint(GTK_WINDOW(view->popup), TRUE);
   gtk_container_set_border_width (GTK_CONTAINER (view->popup), 5);
   frm = gtk_frame_new(NULL);
   gtk_frame_set_shadow_type(GTK_FRAME(frm), GTK_SHADOW_OUT);
   gtk_container_add(GTK_CONTAINER(view->popup), frm);
   border = gtk_container_get_border_width(GTK_CONTAINER(view->popup));
   gtk_container_set_border_width(GTK_CONTAINER(view->popup), 0);
   vbx = gtk_vbox_new(FALSE, 1);
   gtk_container_set_border_width(GTK_CONTAINER(vbx), border);
   gtk_container_add(GTK_CONTAINER(frm), vbx);
   /* handle ESC */
   g_signal_connect(view->popup, "key-press-event",
                            G_CALLBACK(hview_cb_key_pressed), view);

   // subtitle
   lbl = gtk_label_new(_("What goes on?"));
   gtk_container_add(GTK_CONTAINER(vbx), lbl);

   // entry
   view->entry = gtk_entry_new();
   completion = gtk_entry_completion_new();
   g_signal_connect(completion, "match-selected",
                           G_CALLBACK(hview_cb_match_select), NULL);
   g_signal_connect(view->entry, "activate",
                           G_CALLBACK(hview_cb_entry_activate), view);
   gtk_entry_completion_set_text_column(completion, 0);
   gtk_entry_completion_set_model(completion, GTK_TREE_MODEL(view->storeActivities));
   gtk_container_add(GTK_CONTAINER(vbx), view->entry);
   gtk_entry_set_completion(GTK_ENTRY(view->entry), completion);

   // label
   lbl = gtk_label_new(_("Todays activities"));
   gtk_container_add(GTK_CONTAINER(vbx), lbl);

   // tree view
   view->treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(view->storeFacts));
   gtk_widget_set_has_tooltip(view->treeview, TRUE);
   gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(view->treeview), FALSE);
   gtk_tree_view_set_hover_selection(GTK_TREE_VIEW(view->treeview), TRUE);
   gtk_tree_view_set_grid_lines(GTK_TREE_VIEW(view->treeview), GTK_TREE_VIEW_GRID_LINES_NONE);
   g_signal_connect(view->treeview, "query-tooltip",
                           G_CALLBACK(hview_cb_tv_query_tooltip), view);
   g_signal_connect(view->treeview, "button-press-event",
                           G_CALLBACK(hview_cb_tv_button_press), view);
   renderer = gtk_cell_renderer_text_new ();
   column = gtk_tree_view_column_new_with_attributes ("Time",
                                                      renderer,
                                                      "text", TIME_SPAN,
                                                      NULL);
   gtk_tree_view_append_column (GTK_TREE_VIEW (view->treeview), column);
   column = gtk_tree_view_column_new_with_attributes ("Name",
                                                      renderer,
                                                      "text", TITLE,
                                                      NULL);
   gtk_tree_view_append_column (GTK_TREE_VIEW (view->treeview), column);
   column = gtk_tree_view_column_new_with_attributes ("Duration",
                                                      renderer,
                                                      "text", DURATION,
                                                      NULL);
   gtk_tree_view_append_column (GTK_TREE_VIEW (view->treeview), column);
   renderer = gtk_cell_renderer_pixbuf_new();
   column = gtk_tree_view_column_new_with_attributes ("ed",
                                                      renderer,
                                                      "stock-id", BTNEDIT,
                                                      NULL);
   g_object_set_data(G_OBJECT(column), "tip", _("Edit activity"));
   gtk_tree_view_append_column (GTK_TREE_VIEW (view->treeview), column);
   column = gtk_tree_view_column_new_with_attributes ("ct",
                                                      renderer,
                                                      "stock-id", BTNCONT,
                                                      NULL);
   g_object_set_data(G_OBJECT(column), "tip", _("Resume activity"));
   gtk_tree_view_append_column (GTK_TREE_VIEW (view->treeview), column);
   gtk_container_add(GTK_CONTAINER(vbx), view->treeview);

   // summary
   gtk_misc_set_alignment(GTK_MISC(view->summary), 1.0, 0.0);
   gtk_label_set_line_wrap(GTK_LABEL(view->summary), TRUE);
   gtk_label_set_justify(GTK_LABEL(view->summary), GTK_JUSTIFY_RIGHT);
   gtk_container_add(GTK_CONTAINER(vbx), view->summary);
   g_signal_connect( G_OBJECT( view->summary ), "size-allocate",
                         G_CALLBACK( hview_cb_label_allocate ), view);

   // menuish buttons
   ovw = gtk_button_new_with_label(_("Show overview"));
   gtk_button_set_relief(GTK_BUTTON(ovw), GTK_RELIEF_NONE);
   gtk_button_set_focus_on_click(GTK_BUTTON(ovw), FALSE);
   gtk_button_set_alignment(GTK_BUTTON(ovw), 0.0, 0.5);
   g_signal_connect(ovw, "clicked",
                           G_CALLBACK(hview_cb_show_overview), view);
   stp = gtk_button_new_with_label(_("Stop tracking"));
   gtk_button_set_relief(GTK_BUTTON(stp), GTK_RELIEF_NONE);
   gtk_button_set_focus_on_click(GTK_BUTTON(stp), FALSE);
   gtk_button_set_alignment(GTK_BUTTON(stp), 0.0, 0.5);
   g_signal_connect(stp, "clicked",
                           G_CALLBACK(hview_cb_stop_tracking), view);
   add = gtk_button_new_with_label(_("Add earlier activity"));
   gtk_button_set_relief(GTK_BUTTON(add), GTK_RELIEF_NONE);
   gtk_button_set_focus_on_click(GTK_BUTTON(add), FALSE);
   gtk_button_set_alignment(GTK_BUTTON(add), 0.0, 0.5);
   g_signal_connect(add, "clicked",
                           G_CALLBACK(hview_cb_add_earlier_activity), view);
   sep = gtk_hseparator_new();
   cfg = gtk_button_new_with_label(_("Tracking settings"));
   gtk_button_set_relief(GTK_BUTTON(cfg), GTK_RELIEF_NONE);
   gtk_button_set_focus_on_click(GTK_BUTTON(cfg), FALSE);
   gtk_button_set_alignment(GTK_BUTTON(cfg), 0.0, 0.5);
   g_signal_connect(cfg, "clicked",
                           G_CALLBACK(hview_cb_tracking_settings), view);

   gtk_box_pack_start(GTK_BOX(vbx), ovw, FALSE, FALSE, 0);
   gtk_box_pack_start(GTK_BOX(vbx), stp, FALSE, FALSE, 0);
   gtk_box_pack_start(GTK_BOX(vbx), add, FALSE, FALSE, 0);
   gtk_box_pack_start(GTK_BOX(vbx), sep, FALSE, TRUE, 0);
   gtk_box_pack_start(GTK_BOX(vbx), cfg, FALSE, FALSE, 0);

   gtk_widget_show_all(view->popup);

   g_signal_connect (G_OBJECT (view->popup),
                    "focus-out-event",
                    G_CALLBACK (hview_cb_popup_focus_out),
                    view);
}

static void
hview_completion_mode_update(HamsterView *view)
{
   if(view->entry && gtk_widget_get_realized(view->entry))
   {
      gboolean dropdown = xfconf_channel_get_bool(view->channel, XFPROP_DROPDOWN,
            FALSE);
      GtkEntryCompletion *completion = gtk_entry_get_completion(
            GTK_ENTRY(view->entry));
      gtk_entry_completion_set_inline_completion(completion, !dropdown);
      gtk_entry_completion_set_popup_completion(completion, dropdown);
   }
}

static void
hview_autohide_mode_update(HamsterView *view)
{
   view->donthide = xfconf_channel_get_bool(view->channel, XFPROP_DONTHIDE,
         FALSE);
}

static void
hview_tooltips_mode_update(HamsterView *view)
{
   view->tooltips = xfconf_channel_get_bool(view->channel, XFPROP_TOOLTIPS,
         TRUE);
}

/* Actions */
static void
hview_popup_show(HamsterView *view)
{
   /* toggle the button */
   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(view->button), TRUE);

   /* check if popup is needed, or it needs an update */
   if (view->popup == NULL)
      hview_popup_new(view);

   /* honor settings */
   hview_completion_mode_update(view);
   hview_tooltips_mode_update(view);

   /* popup popup */
   gtk_window_present_with_time(GTK_WINDOW(view->popup),
         gtk_get_current_event_time());
   gtk_widget_add_events(view->popup, GDK_FOCUS_CHANGE_MASK|GDK_KEY_PRESS_MASK);
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
      gboolean isStopped = FALSE;

      struct tm *tma = gmtime(&act->startTime);
      strftime(spn, 20, "%H:%M", tma);
      strcat(spn, " - ");
      ptr = spn + strlen(spn);
      if(act->endTime)
      {
         tma = gmtime(&act->endTime);
         strftime(ptr, 20, "%H:%M", tma);
         isStopped = TRUE;
      }
      sprintf(dur, "%dh %dmin", act->seconds / 3600, (act->seconds / 60) % 60);

      gtk_list_store_append (view->storeFacts, &iter);  /* Acquire an iterator */
      gtk_list_store_set (view->storeFacts, &iter,
                          TIME_SPAN, spn,
                          TITLE, act->name,
                          DURATION, dur,
                          BTNEDIT, "gtk-edit",
                          BTNCONT, isStopped ? "gtk-media-play" : "",
                          ID, act->id,
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

   if(tbl)
   {
      count = g_hash_table_size(tbl);
      g_hash_table_iter_init(&iter, tbl);
      while(g_hash_table_iter_next(&iter, (gpointer)&cat, (gpointer)&sum))
      {
         count--;
         g_string_append_printf(string, count ? "%s: %d.%1d, " : "%s: %d.%1d ",
               cat, *sum / 3600, (10 * (*sum % 3600)) / 3600);
      }
   }
   else
   {
      g_string_append(string, _("No activities yet."));
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
               gchar *act, *cat, *actlow;
               g_variant_get(dbusAct, "(ss)", &act, &cat);
               actlow = g_utf8_casefold(act, -1);
               gtk_list_store_append(view->storeActivities, &iter);
               gtk_list_store_set(view->storeActivities, &iter,
                     0, actlow, 1, cat, -1);
               g_free(actlow);
            }
         }
      }
   }
}

static void
hview_button_update(HamsterView *view)
{
   GVariant *res;
   gsize count = 0;
   if(NULL != view->storeFacts)
      gtk_list_store_clear(view->storeFacts);
   if(NULL != view->hamster)
   {
      if(hamster_call_get_todays_facts_sync(view->hamster, &res, NULL, NULL))
      {
         if(NULL != res && (count = g_variant_n_children(res)))
         {
            int i;
            GHashTable *tbl = g_hash_table_new(g_str_hash, g_str_equal);
            gtk_widget_set_sensitive(view->treeview, TRUE);
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
   if (!count)
      hview_summary_update(view, NULL);
   gtk_widget_set_sensitive(view->treeview, count > 0);
}

static gboolean
hview_cb_button_pressed(GtkWidget *widget, GdkEventButton *evt, HamsterView *view)
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
      hview_cb_show_overview(NULL, view);
   }
   hview_button_update(view);
   return TRUE;
}

static gboolean
hview_cb_hamster_changed(Hamster *hamster, HamsterView *view)
{
   DBG("dbus-callback %p", view);
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

static void
hview_cb_channel(XfconfChannel *channel,
                 gchar         *property,
                 GValue        *value,
                 HamsterView   *view)
{
   DBG("%s=%d", property, g_value_get_boolean(value));
   if(!strcmp(property, XFPROP_DROPDOWN))
      hview_completion_mode_update(view);
   else if(!strcmp(property, XFPROP_DONTHIDE))
      hview_autohide_mode_update(view);
   else if(!strcmp(property, XFPROP_TOOLTIPS))
      hview_tooltips_mode_update(view);
}

HamsterView*
hamster_view_init(XfcePanelPlugin* plugin)
{
   HamsterView *view;

   g_assert(plugin != NULL);

   view            = g_new0(HamsterView, 1);
   view->plugin    = plugin;
   DBG("initializing %p", view);

   /* init button */

   DBG("init GUI");

   /* create the button */
   view->button = g_object_ref(places_button_new(view->plugin));
   xfce_panel_plugin_add_action_widget(view->plugin, view->button);
   gtk_container_add(GTK_CONTAINER(view->plugin), view->button);
   gtk_widget_show(view->button);

   /* button signal */
   g_signal_connect(view->button, "button-press-event",
                            G_CALLBACK(hview_cb_button_pressed), view);

   g_timeout_add_seconds(60, (GSourceFunc)hview_cb_cyclic, view);

   /* remote control */
   view->hamster = hamster_proxy_new_for_bus_sync
         (
                     G_BUS_TYPE_SESSION,
                     G_DBUS_PROXY_FLAGS_NONE,
                     "org.gnome.Hamster",             /* bus name */
                     "/org/gnome/Hamster",            /* object */
                     NULL,                            /* GCancellable* */
                     NULL);

   g_signal_connect(view->hamster, "facts-changed",
                            G_CALLBACK(hview_cb_hamster_changed), view);
   g_signal_connect(view->hamster, "activities-changed",
                            G_CALLBACK(hview_cb_hamster_changed), view);

   view->windowserver = window_server_proxy_new_for_bus_sync
         (
               G_BUS_TYPE_SESSION,
               G_DBUS_PROXY_FLAGS_NONE,
               "org.gnome.Hamster.WindowServer",      /* bus name */
               "/org/gnome/Hamster/WindowServer",     /* object */
               NULL,                                  /* GCancellable* */
               NULL);

   /* storage */
   view->storeActivities = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
   view->storeFacts = gtk_list_store_new(NUM_COL, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
         G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT);
   view->summary = gtk_label_new(NULL);
   view->treeview = gtk_tree_view_new();

   /* config */
   view->channel = xfce_panel_plugin_xfconf_channel_new(view->plugin);
   g_signal_connect(view->channel, "property-changed",
                        G_CALLBACK(hview_cb_channel), view);
   g_signal_connect(view->plugin, "configure-plugin",
                        G_CALLBACK(config_show), view->channel);
   xfce_panel_plugin_menu_show_configure(view->plugin);

   /* time helpers */
   tzset();

   /* liftoff */
   hview_button_update(view);
   hview_completion_update(view);

   DBG("done");

   return view;
}

void
hamster_view_finalize(HamsterView* view)
{
   g_free(view);
}


