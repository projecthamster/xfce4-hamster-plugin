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

#pragma once
#include <glib.h>
#include <gtk/gtk.h>

typedef struct _fact
{
   int id; // 0
   time_t startTime; // 1
   time_t endTime; // 2
   char *description; // 3
   char *name; // 4
   int activityId; // 5
   char *category; // 6
   char **tags; // 7
   time_t date; // 8
   int seconds; // 9
}fact;

fact*
fact_new(GVariant *in);

void
fact_free(fact *in);
