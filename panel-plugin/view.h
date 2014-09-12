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

/* vim: set ai et tabstop=4: */

