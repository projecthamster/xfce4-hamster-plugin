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

#include "util.h"

fact *
fact_new(GVariant *in)
{
   fact *out = g_new0(fact, 1);
   g_variant_get(in, "(iiissisasii)",
         &out->id,
         &out->startTime,
         &out->endTime,
         &out->description,
         &out->name,
         &out->activityId,
         &out->category,
         &out->tags,
         &out->date,
         &out->seconds
         );
   return out;
}

void
fact_free(fact *in)
{
   g_free(in->description);
   g_free(in->name);
   g_free(in->category);
   // must not free in->tags
   g_free(in);
}
