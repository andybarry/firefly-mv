/*
 * Use Gtk+ to show live video from the dc1394 camera
 *
 * Written by John Stowers <john.stowers@gmail.com>
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
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>

#include <glib.h>
#include <gtk/gtk.h>
#include <dc1394/dc1394.h>

#include "camera.h"
#include "utils.h"
#include "gtkutils.h"

#define FPS_TO_MS(x) ((1.0/x)*1000.0)

typedef struct __view
{
    show_mode_t             show;
    dc1394video_frame_t     *frame;
    dc1394camera_t          *camera;
} view_t;

static gboolean delete_event( GtkWidget *widget, GdkEvent *event, gpointer data )
{
    gtk_main_quit();
    return TRUE;
}

static gboolean expose_event_callback (GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
    dc1394error_t err;
    view_t *view = (view_t *)data;

    err=dc1394_capture_dequeue(view->camera, DC1394_CAPTURE_POLICY_WAIT, &(view->frame));
    DC1394_WRN(err,"Could not capture a frame");

    render_frame_to_widget(view->frame, widget, view->show);
        
    err=dc1394_capture_enqueue(view->camera, view->frame);
    DC1394_WRN(err,"releasing buffer");

    return TRUE;
}

static gboolean redraw(gpointer data)
{
    gtk_widget_queue_draw(GTK_WIDGET(data));
    return TRUE;
}

int main(int argc, char *argv[])
{
    dc1394_t * d;
    dc1394error_t err;
    unsigned int width, height;
    GtkWidget *window, *canvas;
    char *format = NULL;
    double framerate;
    int exposure, brightness;
    view_t view;
    guint64 guid;

    /* Option parsing */
    GError *error = NULL;
    GOptionContext *context;
    GOptionEntry entries[] =
    {
      GOPTION_ENTRY_FORMAT(&format),
      GOPTION_ENTRY_CAMERA_SETUP_ARGUMENTS(&guid, &framerate, &exposure, &brightness),
      { NULL }
    };

    context = g_option_context_new("- Firefly MV Camera Viewer");
    g_option_context_set_summary(context, "Shows live video from the selected camera");
    g_option_context_add_main_entries (context, entries, NULL);

    /* Defaults */
    guid = MY_CAMERA_GUID;
    view.show = GRAY;
    framerate = 30.0;
    exposure = -1;
    brightness = -1;

    if (!g_option_context_parse (context, &argc, &argv, &error)) {
        printf( "Error: %s\n%s", 
                error->message, 
                g_option_context_get_help(context, TRUE, NULL));
        exit(1);
    }
    if (format && format[0])
        view.show = format[0];

    switch (view.show) {
        case GRAY:
        case COLOR:
        case FORMAT7:
            printf("Selected mode: %c\n", view.show);
            break;
        default:
            app_exit(1, context, "Invalid Mode"); 
    }
    
    d = dc1394_new ();
    if (!d)
        app_exit(2, NULL, "Could not initialize libdc1394");

    view.camera = dc1394_camera_new (d, guid);
    if (!view.camera)
        app_exit(3, context, "Could not find or initialize camera");

    gtk_init( &argc, &argv );

    // setup capture
    switch (view.show) {
        case GRAY:
        case COLOR:
            dc1394_get_image_size_from_video_mode(view.camera, DC1394_VIDEO_MODE_640x480_MONO8, &width, &height);
            err=setup_gray_capture(view.camera, DC1394_VIDEO_MODE_640x480_MONO8);
            break;
        case FORMAT7:
            dc1394_get_image_size_from_video_mode(view.camera, DC1394_VIDEO_MODE_FORMAT7_0, &width, &height);
            err=setup_color_capture(view.camera, DC1394_VIDEO_MODE_FORMAT7_0, DC1394_COLOR_CODING_RAW8);
            break;
    }
    DC1394_ERR_CLN_RTN(err,cleanup_and_exit(view.camera),"Could not setup camera");

    err=setup_from_command_line(view.camera, framerate, exposure, brightness);
    DC1394_ERR_CLN_RTN(err,cleanup_and_exit(view.camera),"Could not set camera from command line arguments");


    // have the camera start sending us data
    err=dc1394_video_set_transmission(view.camera, DC1394_ON);
    DC1394_ERR_CLN_RTN(err,cleanup_and_exit(view.camera),"Could not start camera iso transmission");

    // create window
    gtk_widget_set_default_colormap (gdk_rgb_get_cmap());
    gtk_widget_set_default_visual (gdk_rgb_get_visual());
    window = gtk_window_new( GTK_WINDOW_TOPLEVEL );
    g_signal_connect( 
            G_OBJECT(window), "delete_event", 
            G_CALLBACK(delete_event), NULL );
    gtk_container_set_border_width( GTK_CONTAINER(window), 10 );

    canvas = gtk_drawing_area_new();
    gtk_widget_set_size_request(canvas, width, height);
    g_signal_connect (G_OBJECT (canvas), "expose_event",  
            G_CALLBACK (expose_event_callback), &view);

    gtk_container_add( GTK_CONTAINER(window), canvas );

    // setup refresh rate
    g_timeout_add (FPS_TO_MS(20), redraw, canvas);

    // go
    gtk_widget_show_all( window );
    gtk_main();

    // stop data transmission
    err=dc1394_video_set_transmission(view.camera,DC1394_OFF);
    DC1394_ERR_CLN_RTN(err,cleanup_and_exit(view.camera),"Could not stop the camera");

    // close camera
    cleanup_and_exit(view.camera);
    dc1394_free (d);

    return 0;
}
