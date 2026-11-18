/*  xfce4-hamster-plugin
 *
 *  Copyright (c) 2026 Hakan Erduman <hakan@erduman.de>
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
 *  along with this program; If not, see <https://www.gnu.org/licenses/>.
 */

/* Screenshot comparison for the live Wayland panel test.
 *
 * The panel strip at the top of the screen changes on hover, so callers pass
 * --changed/--unchanged a BELOW row and only that part is compared. RGB is
 * compared with a small per-channel tolerance; alpha is ignored because grim
 * screenshots are opaque. Prints "bbox=x,y,w,h" or "bbox=none" on stdout so
 * the shell test can assert the popup anchor.
 *
 * usage: popup-image-diff --changed BELOW MINW MINH A B
 *        popup-image-diff --unchanged BELOW A B
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gdk-pixbuf/gdk-pixbuf.h>

/* Per-channel tolerance. Identical screenshots differ by zero; a tolerance
 * absorbs PNG encoder or colour-conversion noise without hiding a popup. */
#define CHANNEL_THRESHOLD 8

static int parse_int(const char *text, int *value)
{
   char *end = NULL;
   long parsed;

   parsed = strtol(text, &end, 10);
   if (end == text || *end != '\0' || parsed < 0 || parsed > 1000000)
   {
      return 0;
   }
   *value = (int) parsed;
   return 1;
}

static void usage(const char *program)
{
   fprintf(stderr,
           "usage: %s --changed BELOW MINW MINH A B\n"
           "       %s --unchanged BELOW A B\n",
           program, program);
}

static GdkPixbuf *load_image(const char *path)
{
   GError *error = NULL;
   GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(path, &error);

   if (pixbuf == NULL)
   {
      fprintf(stderr, "popup-image-diff: cannot load %s: %s\n", path,
              error != NULL ? error->message : "unknown error");
      if (error != NULL)
      {
         g_error_free(error);
      }
   }
   return pixbuf;
}

/* Bounding box of pixels that differ in RGB and lie at or below BELOW.
 * Returns 0 when there is no such pixel. */
static int diff_bbox(GdkPixbuf *a, GdkPixbuf *b, int below,
                     int *box_x, int *box_y, int *box_w, int *box_h)
{
   int width = gdk_pixbuf_get_width(a);
   int height = gdk_pixbuf_get_height(a);
   int channels_a = gdk_pixbuf_get_n_channels(a);
   int channels_b = gdk_pixbuf_get_n_channels(b);
   int stride_a = gdk_pixbuf_get_rowstride(a);
   int stride_b = gdk_pixbuf_get_rowstride(b);
   const guchar *pixels_a = gdk_pixbuf_get_pixels(a);
   const guchar *pixels_b = gdk_pixbuf_get_pixels(b);
   int min_x = width;
   int min_y = height;
   int max_x = -1;
   int max_y = -1;
   int x;
   int y;

   if (below < 0)
   {
      below = 0;
   }

   for (y = below; y < height; y++)
   {
      const guchar *row_a = pixels_a + (size_t) y * stride_a;
      const guchar *row_b = pixels_b + (size_t) y * stride_b;

      for (x = 0; x < width; x++)
      {
         const guchar *pa = row_a + (size_t) x * channels_a;
         const guchar *pb = row_b + (size_t) x * channels_b;
         int changed = 0;
         int channel;

         for (channel = 0; channel < 3; channel++)
         {
            int difference = (int) pa[channel] - (int) pb[channel];

            if (difference < 0)
            {
               difference = -difference;
            }
            if (difference > CHANNEL_THRESHOLD)
            {
               changed = 1;
               break;
            }
         }

         if (changed)
         {
            if (x < min_x)
            {
               min_x = x;
            }
            if (y < min_y)
            {
               min_y = y;
            }
            if (x > max_x)
            {
               max_x = x;
            }
            if (y > max_y)
            {
               max_y = y;
            }
         }
      }
   }

   if (max_x < 0)
   {
      return 0;
   }

   *box_x = min_x;
   *box_y = min_y;
   *box_w = max_x - min_x + 1;
   *box_h = max_y - min_y + 1;
   return 1;
}

int main(int argc, char **argv)
{
   GdkPixbuf *a;
   GdkPixbuf *b;
   int mode_changed;
   int below;
   int min_w = 0;
   int min_h = 0;
   int box_x = 0;
   int box_y = 0;
   int box_w = 0;
   int box_h = 0;
   int found;

   if (argc == 7 && strcmp(argv[1], "--changed") == 0)
   {
      mode_changed = 1;
      if (!parse_int(argv[2], &below) || !parse_int(argv[3], &min_w)
          || !parse_int(argv[4], &min_h))
      {
         usage(argv[0]);
         return 2;
      }
   }
   else if (argc == 5 && strcmp(argv[1], "--unchanged") == 0)
   {
      mode_changed = 0;
      if (!parse_int(argv[2], &below))
      {
         usage(argv[0]);
         return 2;
      }
   }
   else
   {
      usage(argv[0]);
      return 2;
   }

   a = load_image(argv[argc - 2]);
   if (a == NULL)
   {
      return 2;
   }
   b = load_image(argv[argc - 1]);
   if (b == NULL)
   {
      g_object_unref(a);
      return 2;
   }

   if (gdk_pixbuf_get_width(a) != gdk_pixbuf_get_width(b)
       || gdk_pixbuf_get_height(a) != gdk_pixbuf_get_height(b))
   {
      fprintf(stderr,
              "popup-image-diff: size mismatch: %dx%d vs %dx%d\n",
              gdk_pixbuf_get_width(a), gdk_pixbuf_get_height(a),
              gdk_pixbuf_get_width(b), gdk_pixbuf_get_height(b));
      g_object_unref(a);
      g_object_unref(b);
      return 2;
   }

   /* RGB comparison needs at least three channels in both images. */
   if (gdk_pixbuf_get_n_channels(a) < 3 || gdk_pixbuf_get_n_channels(b) < 3)
   {
      fprintf(stderr, "popup-image-diff: unsupported pixel format "
                      "(%d and %d channels)\n",
              gdk_pixbuf_get_n_channels(a), gdk_pixbuf_get_n_channels(b));
      g_object_unref(a);
      g_object_unref(b);
      return 2;
   }

   found = diff_bbox(a, b, below, &box_x, &box_y, &box_w, &box_h);
   g_object_unref(a);
   g_object_unref(b);

   if (!found)
   {
      printf("bbox=none\n");
      /* --unchanged succeeds on an unchanged area; --changed needs a box. */
      return mode_changed ? 1 : 0;
   }

   printf("bbox=%d,%d,%d,%d\n", box_x, box_y, box_w, box_h);
   if (!mode_changed)
   {
      return 1;
   }
   return (box_w >= min_w && box_h >= min_h) ? 0 : 1;
}
