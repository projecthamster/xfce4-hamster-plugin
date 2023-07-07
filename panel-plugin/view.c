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
   XfcePanelPlugin *plugin;

   /* view */
   GtkWidget *button;
   GtkWidget *popup;
   GtkWidget *vbx;
   GtkWidget *entry;
   GtkWidget *treeview;
   GtkWidget *summary;
   gboolean   alive;
   guint      sourceTimeout;
   gboolean   inCellEdit;

   /* model */
   GtkListStore       *storeFacts;
   GtkListStore       *storeActivities;
   Hamster            *hamster;
   WindowServer       *windowserver;
   GtkEntryCompletion *completion;

   /* config */
   XfconfChannel *channel;
   gboolean       donthide;
   gboolean       tooltips;
   gboolean       dropdown;
};

enum _HamsterViewColumns
{
   TIME_SPAN,   // 09:00 - 10:15
   TITLE,       // namedchore, without category
   DURATION,    // 0h 56min
   BTNEDIT,     //
   BTNCONT,     //
   ID,          // hidden, int
   CATEGORY,    // hidden, int
   NUM_COL      // not a column, sentinel
};

const int secsperhour = 3600;
const int secspermin  = 60;

/* Button */
static void hview_popup_hide(HamsterView *view)
{
   if (view->donthide)
   {
      return;
   }
   /* untoggle the button */
   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(view->button), FALSE);

   /* empty entry */
   if (view->entry)
   {
      gtk_entry_set_text(GTK_ENTRY(view->entry), "");
      gtk_widget_grab_focus(view->entry);
   }

   /* hide the dialog for reuse */
   if (view->popup)
   {
      gtk_widget_hide(view->popup);
   }
   view->alive      = FALSE;
   view->inCellEdit = FALSE;
}

/* Button callbacks */
static void hview_cb_show_overview(GtkWidget *widget, HamsterView *view)
{
   DBG("cb:%s", gtk_widget_get_name(widget));
   window_server_call_overview_sync(view->windowserver, NULL, NULL);
   hview_popup_hide(view);
}

static void hview_cb_stop_tracking(GtkWidget *widget, HamsterView *view)
{
   DBG("cb:%s", gtk_widget_get_name(widget));
   time_t    now  = time(NULL);
   struct tm tmlt = { 0 };
   localtime_r(&now, &tmlt);
   now -= timezone;
   if (tmlt.tm_isdst)
   {
      now += (daylight * secsperhour);
   }
   GVariant *stopTime = g_variant_new_int64(now);
   GVariant *var      = g_variant_new_variant(stopTime);
   hamster_call_stop_tracking_sync(view->hamster, var, NULL, NULL);
   hview_popup_hide(view);
}

static void hview_cb_add_earlier_activity(GtkWidget *widget, HamsterView *view)
{
   DBG("cb:%s", gtk_widget_get_name(widget));
   GVariant *_ret = g_dbus_proxy_call_sync(
      G_DBUS_PROXY(view->windowserver), "edit", g_variant_new("()"), G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL);
   if (_ret)
   {
      g_variant_get(_ret, "()");
      g_variant_unref(_ret);
   }
   hview_popup_hide(view);
}

static void hview_cb_tracking_settings(GtkWidget *widget, HamsterView *view)
{
   DBG("cb:%s", gtk_widget_get_name(widget));
   window_server_call_preferences_sync(view->windowserver, NULL, NULL);
   hview_popup_hide(view);
}

static gboolean hview_cb_timeout(HamsterView *view)
{
   hview_popup_hide(view);
   view->sourceTimeout = 0;
   return FALSE;
}

gboolean hview_cb_popup_focus_out(GtkWidget *widget, GdkEventFocus const *event, HamsterView *view)
{
   DBG("cb:%s, in=%d", gtk_widget_get_name(widget), event->in);

   if (view->donthide)
   {
      return FALSE;
   }
   if (!view->sourceTimeout)
   {
      const uint doubleBlinkEye = 50;   // milliseconds
      view->sourceTimeout       = g_timeout_add(doubleBlinkEye, (GSourceFunc)hview_cb_timeout, view);
      return FALSE;
   }
   return TRUE;
}

static gboolean hview_cb_match_select(GtkEntryCompletion *completion,
                                      GtkTreeModel       *model,
                                      GtkTreeIter        *iter,
                                      HamsterView        *view)
{
   char *activity;
   char *category;
   char  fact[ MAX_FACT_LEN ];
   int   id = 0;

   DBG("cb:%s", gtk_widget_get_name(GTK_WIDGET(completion)));
   gtk_tree_model_get(model, iter, 0, &activity, 1, &category, -1);
   snprintf(fact, sizeof(fact), "%s@%s", activity, category);
   if (view->inCellEdit)
   {
      DBG("edited: %s", fact);
   }
   else
   {
      hamster_call_add_fact_sync(view->hamster, fact, 0, 0, FALSE, &id, NULL, NULL);
      DBG("selected: %s[%d]", fact, id);
   }
   hview_popup_hide(view);
   g_free(activity);
   g_free(category);
   return TRUE;
}

static const char *hview_get_fact_by_activity(HamsterView *view, const char *activity, const char *category)
{
   static char buffer[ MAX_FACT_LEN ];

   if (NULL != category)   // category to be appended
   {
      snprintf(buffer, sizeof(buffer), "%s@%s", activity, category);
      return buffer;
   }

   // category to be appended
   GVariant *res;
   GVariant *child;
   char     *act = NULL;
   char     *cat = NULL;

   hamster_call_get_activities_sync(view->hamster, activity, &res, 0, 0);
   if (NULL != res && g_variant_n_children(res) > 0)
   {
      child = g_variant_get_child_value(res, 0);   // topmost in history is OK
      if (child)
      {
         g_variant_get(child, "(ss)", &act, &cat);
         if (NULL != act && NULL != cat)
         {
            snprintf(buffer, sizeof(buffer), "%s@%s", act, cat);
            activity = buffer;
         }
         g_variant_unref(child);
      }
   }   // else category stays empty

   return activity;
}

static void hview_cb_entry_activate(GtkEntry *entry, HamsterView *view)
{
   DBG("cb:%s", gtk_widget_get_name(GTK_WIDGET(entry)));
   const char *fact = gtk_entry_get_text(GTK_ENTRY(view->entry));
   int         id   = 0;

   if (!strchr(fact, '@'))
   {
      fact = hview_get_fact_by_activity(view, fact, NULL);
   }

   hamster_call_add_fact_sync(view->hamster, fact, 0, 0, FALSE, &id, NULL, NULL);
   DBG("activated: %s[%d]", fact, id);
   hview_popup_hide(view);
}

static gboolean hview_cb_tv_query_tooltip(GtkWidget         *widget,
                                          gint               x,
                                          gint               y,
                                          unused gboolean    keyboard_mode,
                                          GtkTooltip        *tooltip,
                                          const HamsterView *view)
{
   if (view->tooltips)
   {
      GtkTreePath       *path;
      GtkTreeViewColumn *column;
      if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(widget), x, y, &path, &column, NULL, NULL))
      {
         gchar const *text;
         if (NULL != (text = g_object_get_data(G_OBJECT(column), "tip")))
         {
            gtk_tooltip_set_text(tooltip, text);
            return TRUE;
         }
      }
   }
   return FALSE;
}

static gboolean hview_cb_tv_button_press(GtkWidget *tv, const GdkEventButton *evt, HamsterView *view)
{
   if (evt->button != 1)
   {
      return FALSE;
   }

   GtkTreePath       *path;
   GtkTreeViewColumn *column;
   if (!gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(tv), (int)evt->x, (int)evt->y, &path, &column, NULL, NULL))
   {
      return FALSE;
   }

   GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tv));
   GtkTreeModel     *model     = NULL;
   GtkTreeIter       iter;
   if (gtk_tree_selection_get_selected(selection, &model, &iter))
   {
      int   id;
      char *icon;
      char *fact;
      char *category;
      gtk_tree_model_get(model, &iter, ID, &id, BTNCONT, &icon, TITLE, &fact, CATEGORY, &category, -1);
      DBG("%s:%s:%s", fact, category, icon);
      if (!strcmp(gtk_tree_view_column_get_title(column), "ed"))
      {
         GVariant *dummy = g_variant_new_int32(id);
         GVariant *var   = g_variant_new_variant(dummy);
         window_server_call_edit_sync(view->windowserver, var, NULL, NULL);
      }
      else if (!strcmp(gtk_tree_view_column_get_title(column), "ct") && !strcmp(icon, "gtk-media-play"))
      {
         char fact_at_category[ MAX_FACT_LEN ];
         snprintf(fact_at_category, sizeof(fact_at_category), "%s@%s", fact, category);
         DBG("Resume %s", fact_at_category);
         hamster_call_add_fact_sync(view->hamster, fact_at_category, 0, 0, FALSE, &id, NULL, NULL);
      }
      g_free(icon);
      g_free(fact);
      g_free(category);
   }
   gtk_tree_path_free(path);

   return FALSE;
}

static void hview_cb_label_allocate(GtkWidget *label, unused const GtkAllocation *allocation, HamsterView *view)
{
   if (gtk_widget_get_sensitive(view->treeview))
   {
      GtkRequisition req;
      GtkRequisition dummy;
      gtk_widget_get_preferred_size(view->treeview, &req, &dummy);
      if (req.width > 0)
      {
         gtk_widget_set_size_request(label, req.width, -1);
      }
   }
   else
   {
      gtk_widget_set_size_request(label, -1, -1);
   }
}

static gboolean hview_cb_key_pressed(unused const GtkWidget *widget, const GdkEventKey *evt, HamsterView *view)
{
   if (evt->keyval == GDK_KEY_Escape && !view->inCellEdit)
   {
      hview_popup_hide(view);
   }

   return FALSE;
}

void hview_cb_style_set(unused const GtkWidget *widget, unused const GtkStyle *previous, HamsterView *view)
{
   const guint border = 5;
   DBG("style-set %d", border);
   gtk_container_set_border_width(GTK_CONTAINER(view->vbx), border);
}

static gboolean hview_span_to_times(const char *duration, time_t *start_time, time_t *end_time)
{
   gboolean rVal = FALSE;

   if (NULL != start_time && NULL != end_time)
   {
      struct tm sm;
      struct tm em;
      time_t    now;

      time(&now);
      localtime_r(&now, &sm);
      em = sm;
      switch (   // parse '09:30 - 14:30', accept '09:30' or '09:30 -' for ongoing
         sscanf(duration, "%02d:%02d - %02d:%02d", &sm.tm_hour, &sm.tm_min, &em.tm_hour, &em.tm_min))
      {
         case 2:   // hh:mm OK
            *start_time = timegm(&sm);
            *end_time   = 0;   // hamster accepts 0 as ongoing
            rVal        = (-1 != *start_time);
            break;

         case 4:   // hh:mm - hh:mm OK
            *start_time = timegm(&sm);
            *end_time   = timegm(&em);
            rVal        = (-1 != *start_time);
            rVal &= (-1 != *end_time);
            rVal &= (*end_time > *start_time);
            break;

         default:
            *start_time = -1;
            *end_time   = -1;
            break;
      }
   }
   return rVal;
}

static gboolean hview_editing_cancelled(GtkCellEditable *cell_editable)
{
   gboolean cancelled;
   g_object_get(cell_editable, "editing-canceled", &cancelled, NULL);
   return cancelled;
}

static void hview_fact_update(HamsterView *view, int *id, const char *fact, gint start_time, gint stop_time)
{
   // hamster #671 merged upstream?
   if (!hamster_call_update_fact_sync(view->hamster, *id, fact, start_time, stop_time, FALSE, id, NULL, NULL))
   {
      DBG("UpdateFact did not work: remove, then add fact #%d", *id);
      hamster_call_remove_fact_sync(view->hamster, *id, NULL, NULL);
      hamster_call_add_fact_sync(view->hamster, fact, start_time, stop_time, FALSE, id, NULL, NULL);
      DBG("UpdateFact worked: %d", *id);
      return;
   }
   DBG("UpdateFact worked: new id=%d", *id);
}


static void hview_cb_editing_done(GtkCellEditable *cell_editable, HamsterView *view)
{
   view->inCellEdit = FALSE;

   if (!GTK_IS_ENTRY(cell_editable))
   {
      return;
   }

   if (hview_editing_cancelled(cell_editable))
   {
      return;
   }

   GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view->treeview));
   GtkTreeModel     *model     = NULL;
   GtkTreeIter       iter;
   if (!gtk_tree_selection_get_selected(selection, &model, &iter))
   {
      return;
   }
   int         id;
   const char *fact;
   const char *category;
   const char *duration;
   time_t      start_time;
   time_t      stop_time;
   gtk_tree_model_get(model, &iter, ID, &id, TIME_SPAN, &duration, TITLE, &fact, CATEGORY, &category, -1);

   DBG("old: %d:%s:%s:%s", id, duration, fact, category);

   const char *type = (const char *)g_object_get_data(G_OBJECT(cell_editable), "type");
   const char *val  = gtk_entry_get_text(GTK_ENTRY(cell_editable));
   if (!strcmp("fact", type))
   {
      if (!strcmp(fact, val))
      {
         DBG("No change ACK of fact");
         return;
      }

      fact = hview_get_fact_by_activity(view, val, NULL);
   }
   else if (!strcmp("date", type))
   {
      if (!strcmp(duration, val))
      {
         DBG("No change ACK of duration");
         return;
      }

      fact     = hview_get_fact_by_activity(view, fact, category);
      duration = val;
   }
   if (hview_span_to_times(duration, &start_time, &stop_time))
   {
      hview_fact_update(view, &id, fact, (gint)start_time, (gint)stop_time);
   }
   else
   {
      DBG("Duration '%s' did not parse.", duration);
   }
}

static GtkEntryCompletion *hview_completion_new(HamsterView *view, GtkEntry *entry)
{
   GtkEntryCompletion *completion = gtk_entry_completion_new();
   g_signal_connect(completion, "match-selected", G_CALLBACK(hview_cb_match_select), view);
   gtk_entry_completion_set_text_column(completion, 0);
   gtk_entry_completion_set_model(completion, GTK_TREE_MODEL(view->storeActivities));
   gtk_entry_set_completion(entry, completion);
   gtk_entry_completion_set_inline_completion(completion, !view->dropdown);
   gtk_entry_completion_set_popup_completion(completion, view->dropdown);

   return completion;
}

static void hview_cb_cell_editing_started(GtkCellRenderer *cell,
                                          GtkCellEditable *editable,
                                          const gchar     *path,
                                          HamsterView     *view)
{
   if (GTK_IS_ENTRY(editable))
   {
      GtkEntry *entry = GTK_ENTRY(editable);
      char     *type  = g_object_get_data(G_OBJECT(cell), "type");
      DBG("path=%s type=%s", path, type);
      g_object_set_data_full(G_OBJECT(editable), "path", g_strdup(path), g_free);
      g_object_set_data(G_OBJECT(editable), "type", type);
      if (0 == strcmp("fact", type) && NULL == gtk_entry_get_completion(entry))
      {
         hview_completion_new(view, entry);
      }

      g_signal_connect(entry, "editing-done", G_CALLBACK(hview_cb_editing_done), view);

      view->inCellEdit = TRUE;
   }
}

static void hview_popup_new(HamsterView *view)
{
   GtkWidget         *frm;
   GtkWidget         *lbl;
   GtkWidget         *ovw;
   GtkWidget         *stp;
   GtkWidget         *add;
   GtkWidget         *cfg;
   GtkCellRenderer   *renderer;
   GtkTreeViewColumn *column;


   /* Create a new popup */
   view->popup = gtk_window_new(GTK_WINDOW_TOPLEVEL);
   gtk_window_set_type_hint(GTK_WINDOW(view->popup), GDK_WINDOW_TYPE_HINT_UTILITY);
   gtk_window_set_decorated(GTK_WINDOW(view->popup), FALSE);
   gtk_window_set_resizable(GTK_WINDOW(view->popup), FALSE);
   gtk_window_set_position(GTK_WINDOW(view->popup), GTK_WIN_POS_MOUSE);
   gtk_window_set_screen(GTK_WINDOW(view->popup), gtk_widget_get_screen(view->button));
   gtk_window_set_skip_pager_hint(GTK_WINDOW(view->popup), TRUE);
   gtk_window_set_skip_taskbar_hint(GTK_WINDOW(view->popup), TRUE);
   gtk_window_set_keep_above(GTK_WINDOW(view->popup), TRUE);
   gtk_window_stick(GTK_WINDOW(view->popup));

   frm = gtk_frame_new(NULL);
   gtk_frame_set_shadow_type(GTK_FRAME(frm), GTK_SHADOW_OUT);
   gtk_container_add(GTK_CONTAINER(view->popup), frm);
   gtk_container_set_border_width(GTK_CONTAINER(view->popup), 0);
   view->vbx = gtk_box_new(GTK_ORIENTATION_VERTICAL, 1);
   gtk_container_add(GTK_CONTAINER(frm), view->vbx);
   /* handle ESC */
   g_signal_connect(view->popup, "key-press-event", G_CALLBACK(hview_cb_key_pressed), view);

   // subtitle
   lbl = gtk_label_new(_("What goes on?"));
   gtk_container_add(GTK_CONTAINER(view->vbx), lbl);

   // entry
   view->entry      = gtk_entry_new();
   view->completion = hview_completion_new(view, GTK_ENTRY(view->entry));
   g_object_set_data(G_OBJECT(view->entry), "type", "new");
   g_signal_connect(view->completion, "match-selected", G_CALLBACK(hview_cb_match_select), view);
   g_signal_connect(view->entry, "activate", G_CALLBACK(hview_cb_entry_activate), view);
   gtk_container_add(GTK_CONTAINER(view->vbx), view->entry);

   // label
   lbl = gtk_label_new(_("Today's activities"));
   gtk_container_add(GTK_CONTAINER(view->vbx), lbl);

   // tree view
   view->treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(view->storeFacts));
   gtk_widget_set_has_tooltip(view->treeview, TRUE);
   gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(view->treeview), FALSE);
   gtk_tree_view_set_hover_selection(GTK_TREE_VIEW(view->treeview), TRUE);
   gtk_widget_set_can_focus(view->treeview, TRUE);

   gtk_tree_view_set_grid_lines(GTK_TREE_VIEW(view->treeview), GTK_TREE_VIEW_GRID_LINES_NONE);
   g_signal_connect(view->treeview, "query-tooltip", G_CALLBACK(hview_cb_tv_query_tooltip), view);
   g_signal_connect(view->treeview, "button-release-event", G_CALLBACK(hview_cb_tv_button_press), view);
   // time column
   renderer = gtk_cell_renderer_text_new();
   g_object_set(renderer, "editable", TRUE, NULL);
   g_object_set_data(G_OBJECT(renderer), "type", "date");

   g_signal_connect(renderer, "editing-started", G_CALLBACK(hview_cb_cell_editing_started), view);

   column = gtk_tree_view_column_new_with_attributes("Time", renderer, "text", TIME_SPAN, NULL);
   gtk_tree_view_append_column(GTK_TREE_VIEW(view->treeview), column);

   // fact column
   renderer = gtk_cell_renderer_text_new();
   g_object_set(renderer, "editable", TRUE, NULL);
   g_object_set_data(G_OBJECT(renderer), "type", "fact");
   g_signal_connect(renderer, "editing-started", G_CALLBACK(hview_cb_cell_editing_started), view);
   column = gtk_tree_view_column_new_with_attributes("Name", renderer, "text", TITLE, NULL);
   gtk_tree_view_append_column(GTK_TREE_VIEW(view->treeview), column);

   // duration column
   renderer = gtk_cell_renderer_text_new();   // back to readonly, calculated
   column   = gtk_tree_view_column_new_with_attributes("Duration", renderer, "text", DURATION, NULL);
   gtk_tree_view_append_column(GTK_TREE_VIEW(view->treeview), column);
   gtk_tree_view_column_set_clickable(column, FALSE);

   // button columns
   renderer = gtk_cell_renderer_pixbuf_new();
   column   = gtk_tree_view_column_new_with_attributes("ed", renderer, "stock-id", BTNEDIT, NULL);
   g_object_set_data(G_OBJECT(column), "tip", _("Edit activity"));
   gtk_tree_view_append_column(GTK_TREE_VIEW(view->treeview), column);
   column = gtk_tree_view_column_new_with_attributes("ct", renderer, "stock-id", BTNCONT, NULL);
   g_object_set_data(G_OBJECT(column), "tip", _("Resume activity"));
   gtk_tree_view_append_column(GTK_TREE_VIEW(view->treeview), column);
   gtk_container_add(GTK_CONTAINER(view->vbx), view->treeview);

   // summary
   gtk_widget_set_halign(view->summary, GTK_ALIGN_END);
   gtk_widget_set_valign(view->summary, GTK_ALIGN_START);
   gtk_label_set_line_wrap(GTK_LABEL(view->summary), TRUE);
   gtk_label_set_justify(GTK_LABEL(view->summary), GTK_JUSTIFY_RIGHT);
   gtk_container_add(GTK_CONTAINER(view->vbx), view->summary);
   g_signal_connect(G_OBJECT(view->summary), "size-allocate", G_CALLBACK(hview_cb_label_allocate), view);

   // menuish buttons
   ovw = gtk_button_new_with_label(_("Show overview"));
   gtk_widget_set_halign(gtk_bin_get_child(GTK_BIN(ovw)), GTK_ALIGN_START);
   gtk_button_set_relief(GTK_BUTTON(ovw), GTK_RELIEF_NONE);
   gtk_widget_set_focus_on_click(ovw, FALSE);
   g_signal_connect(ovw, "clicked", G_CALLBACK(hview_cb_show_overview), view);

   stp = gtk_button_new_with_label(_("Stop tracking"));
   gtk_widget_set_halign(gtk_bin_get_child(GTK_BIN(stp)), GTK_ALIGN_START);
   gtk_button_set_relief(GTK_BUTTON(stp), GTK_RELIEF_NONE);
   gtk_widget_set_focus_on_click(stp, FALSE);
   g_signal_connect(stp, "clicked", G_CALLBACK(hview_cb_stop_tracking), view);

   add = gtk_button_new_with_label(_("Add earlier activity"));
   gtk_widget_set_halign(gtk_bin_get_child(GTK_BIN(add)), GTK_ALIGN_START);
   gtk_button_set_relief(GTK_BUTTON(add), GTK_RELIEF_NONE);
   gtk_widget_set_focus_on_click(add, FALSE);
   g_signal_connect(add, "clicked", G_CALLBACK(hview_cb_add_earlier_activity), view);

   cfg = gtk_button_new_with_label(_("Tracking settings"));
   gtk_widget_set_halign(gtk_bin_get_child(GTK_BIN(cfg)), GTK_ALIGN_START);
   gtk_button_set_relief(GTK_BUTTON(cfg), GTK_RELIEF_NONE);
   gtk_widget_set_focus_on_click(cfg, FALSE);
   g_signal_connect(cfg, "clicked", G_CALLBACK(hview_cb_tracking_settings), view);

   gtk_box_pack_start(GTK_BOX(view->vbx), ovw, FALSE, FALSE, 0);
   gtk_box_pack_start(GTK_BOX(view->vbx), stp, FALSE, FALSE, 0);
   gtk_box_pack_start(GTK_BOX(view->vbx), add, FALSE, FALSE, 0);
   gtk_box_pack_start(GTK_BOX(view->vbx), cfg, FALSE, FALSE, 0);

   gtk_widget_show_all(view->popup);

   g_signal_connect(G_OBJECT(view->popup), "focus-out-event", G_CALLBACK(hview_cb_popup_focus_out), view);

   g_signal_connect(G_OBJECT(view->button), "style-set", G_CALLBACK(hview_cb_style_set), view);

   hview_cb_style_set(view->button, NULL, view);
}

static void hview_completion_mode_update(HamsterView *view)
{
   view->dropdown = xfconf_channel_get_bool(view->channel, XFPROP_DROPDOWN, FALSE);

   if (view->entry && gtk_widget_get_realized(view->entry))
   {
      GtkEntryCompletion *completion = gtk_entry_get_completion(GTK_ENTRY(view->entry));
      gtk_entry_completion_set_inline_completion(completion, !view->dropdown);
      gtk_entry_completion_set_popup_completion(completion, view->dropdown);
   }
}

static void hview_autohide_mode_update(HamsterView *view)
{
   view->donthide = xfconf_channel_get_bool(view->channel, XFPROP_DONTHIDE, FALSE);
}

static void hview_tooltips_mode_update(HamsterView *view)
{
   view->tooltips = xfconf_channel_get_bool(view->channel, XFPROP_TOOLTIPS, TRUE);
}

/* Actions */
void hview_popup_show(HamsterView *view, gboolean atPointer)
{
   int x = 0;
   int y = 0;

   /* check if popup is needed, or it needs an update */
   if (view->popup == NULL)
   {
      DBG("new");
      hview_popup_new(view);
      gtk_widget_realize(view->popup);
      xfce_panel_plugin_take_window(view->plugin, GTK_WINDOW(view->popup));


      /* init properties from xfconf */
      hview_autohide_mode_update(view);
      hview_tooltips_mode_update(view);
      hview_completion_mode_update(view);
   }
   else if (view->alive)
   {
      DBG("alive");
      if (view->donthide)
      {
         gdk_window_raise(gtk_widget_get_window(view->popup));
      }
      if (view->sourceTimeout)
      {
         g_source_remove(view->sourceTimeout);
         view->sourceTimeout = 0;
         gdk_window_raise(gtk_widget_get_window(view->popup));
      }
      return; /* avoid double invocation */
   }
   view->alive = TRUE;

   /* toggle the button */
   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(view->button), TRUE);

   /* honor settings */
   hview_completion_mode_update(view);
   hview_tooltips_mode_update(view);

   /* popup popup */
   if (atPointer)
   {
      DBG("atpointer");
      GdkDisplay *display = gdk_display_get_default();
      GdkSeat    *seat    = gdk_display_get_default_seat(display);
      GdkDevice  *pointer = gdk_seat_get_pointer(seat);

      gdk_device_get_position(pointer, NULL, &x, &y);
   }
   else
   {
      DBG("atpanel");
      GdkWindow *popup  = gtk_widget_get_window(view->popup);
      GdkWindow *button = gtk_widget_get_window(view->button);
      xfce_panel_plugin_position_widget(view->plugin, view->button, NULL, &x, &y);
      switch (xfce_panel_plugin_get_orientation(view->plugin))
      {
         case GTK_ORIENTATION_HORIZONTAL:
            x += gdk_window_get_width(button) / 2;
            x -= gdk_window_get_width(popup) / 2;
            break;

         case GTK_ORIENTATION_VERTICAL:
            y += gdk_window_get_height(button) / 2;
            y -= gdk_window_get_height(popup) / 2;
            break;
      }
   }
   DBG("move x=%d, y=%d", x, y);
   gtk_window_move(GTK_WINDOW(view->popup), x, y);
   gtk_window_present_with_time(GTK_WINDOW(view->popup), gtk_get_current_event_time());
   gtk_widget_add_events(view->popup, GDK_FOCUS_CHANGE_MASK | GDK_KEY_PRESS_MASK);
}

// Hours and minutes format when given INT_MAX produces "596523h 14min", but
// "snprintf" reports max possible output size like below.
const int HOURS_AND_MINUTES_MIN_LENGTH = 16;

static void hview_seconds_to_hours_and_minutes(char *duration, size_t length, int seconds)
{
   snprintf(duration, length, _("%dh %dmin"), seconds / secsperhour, (seconds / secspermin) % secspermin);
}

static size_t hview_time_to_string(char *str, size_t maxsize, time_t time)
{
   struct tm tm;
   gmtime_r(&time, &tm);

   return strftime(str, maxsize, "%H:%M", &tm);
}

// Using less than that may cause output to be truncated.
const int HVIEW_TIMES_TO_SPAN_MIN_BUF_SIZE = 14;

static void hview_times_to_span(char *time_span, size_t maxsize, time_t start_time, time_t end_time)
{
   char  *ptr = time_span;
   size_t charsWritten;

   charsWritten = hview_time_to_string(ptr, maxsize, start_time);
   maxsize -= charsWritten;
   ptr += charsWritten;

   // snprintf() should never return negative value with static string like this.
   int dataWritten = snprintf(ptr, maxsize, " - ");
   maxsize -= dataWritten;
   ptr += dataWritten;

   if (end_time)
   {
      hview_time_to_string(ptr, maxsize, end_time);
   }
}

static gint *hview_create_category(const char *category, GHashTable *categories)
{
   gint *duration_in_seconds = g_new0(gint, 1);
   g_hash_table_insert(categories, strdup(category), duration_in_seconds);

   return duration_in_seconds;
}

static void hview_increment_category_time(gchar const *category, gint duration_in_seconds, GHashTable *categories)
{
   gint *category_duration = g_hash_table_lookup(categories, category);
   if (category_duration == NULL)
   {
      category_duration = hview_create_category(category, categories);
   }

   *category_duration += duration_in_seconds;
}

static gboolean hview_activity_stopped(fact const *activity)
{
   if (activity->endTime)
   {
      return TRUE;
   }

   return FALSE;
}

static void hview_store_update(HamsterView *view, fact *activity, GHashTable *categories)
{
   if (NULL == view->storeFacts)
   {
      return;
   }

   gchar time_span[ HVIEW_TIMES_TO_SPAN_MIN_BUF_SIZE ];
   hview_times_to_span(time_span, sizeof(time_span), activity->startTime, activity->endTime);

   gchar duration[ HOURS_AND_MINUTES_MIN_LENGTH ];
   hview_seconds_to_hours_and_minutes(duration, sizeof(duration), activity->seconds);

   GtkTreeIter iter;
   gtk_list_store_append(view->storeFacts, &iter); /* Acquire an iterator */
   gtk_list_store_set(view->storeFacts,
                      &iter,
                      TIME_SPAN,
                      time_span,
                      TITLE,
                      activity->name,
                      DURATION,
                      duration,
                      BTNEDIT,
                      "gtk-edit",
                      BTNCONT,
                      hview_activity_stopped(activity) ? "gtk-media-play" : "",
                      ID,
                      activity->id,
                      CATEGORY,
                      activity->category,
                      -1);

   hview_increment_category_time(activity->category, activity->seconds, categories);
}

static void hview_summary_update(HamsterView *view, GHashTable *tbl)
{
   GHashTableIter iter;
   GString       *string = g_string_new("");
   gchar         *cat;
   gint          *sum;
   guint          count;

   if (tbl)
   {
      count = g_hash_table_size(tbl);
      gtk_widget_set_sensitive(view->treeview, count > 0);
      g_hash_table_iter_init(&iter, tbl);
      while (g_hash_table_iter_next(&iter, (gpointer)&cat, (gpointer)&sum))
      {
         count--;
         g_string_append_printf(string,
                                count ? "%s: %dh %dmin, " : "%s: %dh %dmin",
                                cat,
                                *sum / secsperhour,
                                (*sum / secspermin) % secspermin);
      }
   }
   else
   {
      g_string_append(string, _("No activities yet."));
      gtk_widget_set_sensitive(view->treeview, FALSE);
   }
   gtk_label_set_label(GTK_LABEL(view->summary), string->str);
   g_string_free(string, TRUE);
}

static void hview_completion_update(HamsterView *view)
{
   GVariant *res;
   if (view->inCellEdit)
   {
      return;
   }

   if (NULL != view->storeActivities)
   {
      gtk_list_store_clear(view->storeActivities);
   }

   if (NULL == view->hamster)
   {
      return;
   }

   if (!hamster_call_get_activities_sync(view->hamster, "", &res, NULL, NULL) || NULL == res)
   {
      return;
   }

   for (gsize i = 0; i < g_variant_n_children(res); i++)
   {
      GtkTreeIter iter;
      GVariant   *dbusAct = g_variant_get_child_value(res, i);
      gchar      *act;
      gchar      *cat;
      gchar      *actlow;
      g_variant_get(dbusAct, "(ss)", &act, &cat);
      actlow = g_utf8_casefold(act, -1);
      gtk_list_store_append(view->storeActivities, &iter);
      gtk_list_store_set(view->storeActivities, &iter, 0, actlow, 1, cat, -1);
      g_free(actlow);
   }
   g_variant_unref(res);
}

static gsize hview_get_facts(HamsterView *view, GVariant **res)
{
   gsize count = 0;
   if (NULL == view->hamster)
   {
      return count;
   }

   if (hamster_call_get_todays_facts_sync(view->hamster, res, NULL, NULL) && NULL != *res)
   {
      count = g_variant_n_children(*res);
   }
   return count;
}

static void hview_label_update(HamsterView *view, fact *last)
{
   gboolean ellipsize = xfconf_channel_get_bool(view->channel, XFPROP_SANITIZE, FALSE);
   places_button_set_ellipsize(PLACES_BUTTON(view->button), ellipsize);

   if (NULL == last)
   {
      places_button_set_label(PLACES_BUTTON(view->button), _("inactive"));
      gtk_window_resize(GTK_WINDOW(view->popup), 1, 1);
      return;
   }

   if (0 == last->endTime)
   {
      gchar label[ MAX_FACT_LEN ];
      snprintf(label,
               sizeof(label),
               "%s %d:%02d",
               last->name,
               last->seconds / secsperhour,
               (last->seconds / secspermin) % secspermin);
      places_button_set_label(PLACES_BUTTON(view->button), label);
      return;
   }
}

static void hview_button_update(HamsterView *view)
{
   if (view->inCellEdit)
   {
      return;
   }

   if (NULL != view->storeFacts)
   {
      gtk_list_store_clear(view->storeFacts);
   }

   GVariant *res;
   gsize     count = hview_get_facts(view, &res);
   if (count)
   {
      GHashTable *tbl = g_hash_table_new(g_str_hash, g_str_equal);
      for (gsize i = 0; i < count; i++)
      {
         GVariant *dbusFact = g_variant_get_child_value(res, i);
         fact     *last     = fact_new(dbusFact);
         g_variant_unref(dbusFact);
         hview_store_update(view, last, tbl);
         if (last->id && i == count - 1)
         {
            hview_summary_update(view, tbl);
            hview_label_update(view, last);
         }
         fact_free(last);
      }
      g_hash_table_unref(tbl);
      return;
   }

   hview_summary_update(view, NULL);
   hview_label_update(view, NULL);
}

static gboolean hview_cb_button_pressed(unused const GtkWidget *widget, const GdkEventButton *evt, HamsterView *view)
{
   /* (it's the way xfdesktop popup does it...) */
   if ((evt->state & GDK_CONTROL_MASK) && !(evt->state & (GDK_MOD1_MASK | GDK_SHIFT_MASK | GDK_MOD4_MASK)))
   {
      return FALSE;
   }

   if (evt->button == 1)
   {
      gboolean isActive = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(view->button));
      if (isActive)
      {
         hview_popup_hide(view);
      }
      else
      {
         hview_popup_show(view, FALSE);
      }
   }
   else if (evt->button == 2)
   {
      hview_cb_show_overview(NULL, view);
   }
   hview_button_update(view);
   return TRUE;
}

static gboolean hview_cb_hamster_changed(Hamster *hamster, HamsterView *view)
{
   DBG("dbus-callback %p->%p", hamster, view);
   hview_button_update(view);
   hview_completion_update(view);
   return FALSE;
}

static gboolean hview_cb_cyclic(HamsterView *view)
{
   hview_button_update(view);
   return TRUE;
}

static void hview_cb_channel(XfconfChannel *channel, gchar *property, GValue const *value, HamsterView *view)
{
   DBG("%s=%d, locked=%d", property, xfconf_channel_is_property_locked(channel, property), g_value_get_boolean(value));
   if (!strcmp(property, XFPROP_DROPDOWN))
   {
      hview_completion_mode_update(view);
   }
   else if (!strcmp(property, XFPROP_DONTHIDE))
   {
      hview_autohide_mode_update(view);
   }
   else if (!strcmp(property, XFPROP_TOOLTIPS))
   {
      hview_tooltips_mode_update(view);
   }
   else if (!strcmp(property, XFPROP_SANITIZE))
   {
      hview_button_update(view);
   }
}

HamsterView *hamster_view_init(XfcePanelPlugin *plugin)
{
   HamsterView *view;

   g_assert(plugin != NULL);

   view         = g_new0(HamsterView, 1);
   view->plugin = plugin;
   DBG("initializing %p", view);

   /* init button */
   DBG("init GUI");

   /* create the button */
   view->button = g_object_ref(places_button_new(view->plugin));
   xfce_panel_plugin_add_action_widget(view->plugin, view->button);
   gtk_container_add(GTK_CONTAINER(view->plugin), view->button);
   gtk_widget_show(view->button);

   /* button signal */
   g_signal_connect(view->button, "button-press-event", G_CALLBACK(hview_cb_button_pressed), view);

   g_timeout_add_seconds(secspermin, (GSourceFunc)hview_cb_cyclic, view);

   /* remote control */
   view->hamster = hamster_proxy_new_for_bus_sync(G_BUS_TYPE_SESSION,
                                                  G_DBUS_PROXY_FLAGS_NONE,
                                                  "org.gnome.Hamster",  /* bus name */
                                                  "/org/gnome/Hamster", /* object */
                                                  NULL,                 /* GCancellable */
                                                  NULL);

   g_signal_connect(view->hamster, "facts-changed", G_CALLBACK(hview_cb_hamster_changed), view);
   g_signal_connect(view->hamster, "activities-changed", G_CALLBACK(hview_cb_hamster_changed), view);

   view->windowserver = window_server_proxy_new_for_bus_sync(G_BUS_TYPE_SESSION,
                                                             G_DBUS_PROXY_FLAGS_NONE,
                                                             "org.gnome.Hamster.WindowServer",  /* bus name */
                                                             "/org/gnome/Hamster/WindowServer", /* object */
                                                             NULL,                              /* GCancellable */
                                                             NULL);

   /* storage */
   view->storeActivities = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
   view->storeFacts      = gtk_list_store_new(
      NUM_COL, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT, G_TYPE_STRING);
   view->summary  = gtk_label_new(NULL);
   view->treeview = gtk_tree_view_new();

   /* config */
   view->channel = xfce_panel_plugin_xfconf_channel_new(view->plugin);
   g_signal_connect(view->channel, "property-changed", G_CALLBACK(hview_cb_channel), view);
   g_signal_connect(view->plugin, "configure-plugin", G_CALLBACK(config_show), view->channel);
   xfce_panel_plugin_menu_show_configure(view->plugin);

   /* time helpers */
   tzset();

   /* liftoff */
   hview_button_update(view);
   hview_completion_update(view);

   DBG("done");

   return view;
}

void hamster_view_finalize(HamsterView *view)
{
   g_free(view);
}
