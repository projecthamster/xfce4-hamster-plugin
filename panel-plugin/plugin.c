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

#include <glib.h>

#include <libxfce4util/libxfce4util.h>
#include <libxfce4panel/libxfce4panel.h>
#include <xfconf/xfconf.h>
#include "view.h"

/**
 * popups remotely.
 */
gboolean
hamster_popup_remote(XfcePanelPlugin *plugin, gchar *name,
      GValue const * value, HamsterView *view)
{
   gboolean atPointer;
   DBG("Popup remote: %s at %s(%d)", name, xfce_panel_plugin_get_name(plugin), xfce_panel_plugin_get_unique_id(plugin));
   atPointer = g_value_get_boolean(value);
   hview_popup_show(view, atPointer);
   return TRUE;
}

/**
 * Cleans up resources.
 */
static void
hamster_finalize(XfcePanelPlugin *plugin, HamsterView *view)
{
    DBG("Finalize: %s(%d)", xfce_panel_plugin_get_name(plugin), xfce_panel_plugin_get_unique_id(plugin));
    hamster_view_finalize(view);
}

/**
 * Initializes the plugin.
 */
static void
hamster_construct(XfcePanelPlugin *plugin)
{
    HamsterView *view;
    /* settings */
    if(!xfconf_init(NULL))
    {
       DBG("no xfconf - can't continue");
       return;
    }

    DBG("Construct: %s(%d)", xfce_panel_plugin_get_name(plugin), xfce_panel_plugin_get_unique_id(plugin));
    view = hamster_view_init(plugin);
    /* Set up i18n */
    xfce_textdomain(GETTEXT_PACKAGE, LOCALEDIR, "UTF-8");

    /* Connect the finalize callback */
    g_signal_connect(plugin, "free-data",
                     G_CALLBACK(hamster_finalize), view);

    /* Connect the remote-event callback */
    g_signal_connect(plugin, "remote-event",
                     G_CALLBACK(hamster_popup_remote), view);

    DBG("done");
}

XFCE_PANEL_PLUGIN_REGISTER(hamster_construct);
