/*
 * Grab an image using libdc1394
 *
 * Written by Damien Douxchamps <ddouxchamps@users.sf.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Description:
 *
 *  An extension to the original grab image sample program.  This demonstrates
 *  how to collect more detailed information on the various modes of the
 *  camera, convert one image format to another, and waiting so that the
 *  camera has time to capture the image before trying to write it out.
 */

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include <glib.h>
#include <gtk/gtk.h>
#include <dc1394/dc1394.h>

#include "camera.h"
#include "utils.h"
#include "gtkutils.h"

#define IMG_FORMAT  "png"

int main( int argc, char *argv[])
{
    char                *filename, *dir;
    FILE                *fp;
    int                 i;
    long                total_frame_size;
    dc1394video_frame_t frame;
    show_mode_t         show;

    /* Option parsing */
    GError              *error = NULL;
    GOptionContext      *context;
    GOptionEntry        entries[] =
    {
      { "input-filename", 'i', 0, G_OPTION_ARG_FILENAME, &filename, "Input filename", "FILE" },
      { "output-dir", 'o', 0, G_OPTION_ARG_FILENAME, &dir, "Output dir", "PATH" },
      { NULL }
    };

    context = g_option_context_new("- Firefly MV Camera Playback");
    g_option_context_set_summary(context, 
            "Saves successive frames previously recorded\n"
            "using dc1394-record to individual " IMG_FORMAT " image files");
    g_option_context_add_main_entries (context, entries, NULL);

    fp = NULL;
    filename = NULL;
    dir = NULL;

    if (!g_option_context_parse (context, &argc, &argv, &error)) {
        printf( "Error: %s\n%s", 
                error->message, 
                g_option_context_get_help(context, TRUE, NULL));
        exit(1);
    }
    if (filename == NULL || dir == NULL) {
        printf( "Error: You must supply a filename and dir\n%s", 
                g_option_context_get_help(context, TRUE, NULL));
        exit(2);
    }

    if (filename[0] == '-') {
        fp = stdin;
    } else {
        fp = fopen(filename, "rb");
    }

    if( fp == NULL ) { 
        perror("opening file");
        exit(1);
    }

    // read the first frame
    total_frame_size = read_frame( &frame, fp);
    if (frame.color_coding == DC1394_COLOR_CODING_MONO8)
        show = GRAY;
    else if (frame.color_coding == DC1394_COLOR_CODING_RGB8)
        show = COLOR;
    else if (frame.color_coding == DC1394_COLOR_CODING_RAW8)
        show = FORMAT7;
    else {
        perror("invalid color coding");
        exit(1);
    }

    g_type_init();
    gdk_rgb_init();

    /* go back to start of file */
    i = 0;
    fseek(fp, 0, SEEK_SET);
    while (!feof(fp)) 
    {
        char *fname;
        GdkPixbuf *pb;

        if (frame.image) {
            free(frame.image);
            frame.image = NULL;
        }

        read_frame(&frame, fp);
        render_frame_to_pixbuf(&frame, &pb, show);

        fname = g_strdup_printf("%s/%u.%s", dir, i, IMG_FORMAT);
        gdk_pixbuf_save (pb, fname, IMG_FORMAT, NULL, NULL);
        g_object_unref(pb);
        free(fname);

        i++;
    }
    printf("Wrote %d frames (%ld)\n", i, total_frame_size);

    fclose(fp);

    return 0;
}
