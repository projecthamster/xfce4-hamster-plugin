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

/* about.c */
void
about_show(XfcePanelPlugin *plugin, HamsterView *view);

/* config.c */
void
config_show(XfcePanelPlugin *plugin, HamsterView *view);

/* plugin.c */
void
hview_popup_show(HamsterView *view, gboolean atPointer);


/* vim: set ai et tabstop=4: */

