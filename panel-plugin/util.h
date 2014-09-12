#pragma once
#include <glib.h>
#include <gtk/gtk.h>

typedef struct _fact
{
   gint id; // 0
   time_t startTime; // 1
   time_t endTime; // 2
   gchar *description; // 3
   gchar *name; // 4
   gint activityId; // 5
   gchar *category; // 6
   gchar **tags; // 7
   time_t date; // 8
   gint seconds; // 9
}fact;

fact*
fact_new(GVariant *in);

void
fact_free(fact *in);
