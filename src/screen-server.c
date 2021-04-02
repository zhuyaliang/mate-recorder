/* Copyright (C) 2020 zhuyaliang https://github.com/zhuyaliang/
 *
 * Ported from kazam 
 * Copyright (C) 2012 David Klasinc <bigwhale@lubica.net>
 *
 * This file is part of mate-recorder.
 *
 * mate-recorder is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * mate-recorder is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with mate-recorder.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <gtk/gtk.h>
#include <sys/sysinfo.h>
#include <locale.h>
#include <glib/gi18n.h>

#include "screen-server.h"
#include "screen-window.h"
#include "config.h"

enum
{
    PROP_0,
    PROP_MANAGE_VERSION,
    PROP_STAGE,
    PROP_FRAMERATE,
    PROP_PIPELINE,
    PROP_FILE_TEMPLATE,
    PROP_DRAW_CURSOR
};

typedef enum
{
  RECORDER_STATE_CLOSED,
  RECORDER_STATE_PAUSE,
  RECORDER_STATE_RECORDING
} RecorderState;

struct _ScreenServerPrivate
{
    GDBusConnection *connection;
    GHashTable      *win_hash;
    GstElement      *pipeline;
    GstElement      *videosrc;
    GstElement      *filter;
    GstElement      *videoconvert;
    GstElement      *videorate;
    GstElement      *videnc;
    GstElement      *mux;
    GstElement      *in_queue;
    GstElement      *out_queue;
    GstElement      *sink;
    GstElement      *file_queue;
    guint            cpu_count;
    RecorderState    state;
    gchar           *file_name;
};

typedef void (call_back) (ScreenServer *, int );

struct video_format
{
    int        id;
    char      *type;
    char      *format;
    call_back *func;
};

static void avi_format_call_back (ScreenServer *ss, int i);
static void vp8enc_format_call_back (ScreenServer *ss, int i);
static void mp4_format_call_back (ScreenServer *ss, int i);
static void mkv_format_call_back (ScreenServer *ss, int i);

static struct video_format video_formats[] =
{
    {0, "avimux", "RAW (AVI)", avi_format_call_back},
    {1, "vp8enc", "VP8 (WEBM)", vp8enc_format_call_back},
    {2, "x264enc", "H264 (MP4)", mp4_format_call_back},
    {3, "x264enc", "H264 (MKV)", mkv_format_call_back},
};

static guint count = sizeof (video_formats)/sizeof (struct video_format);
typedef GDBusMethodInvocation GDBusMth;
static void screen_server_screen_admin_iface_init (ScreenAdminIface *iface);

G_DEFINE_TYPE_WITH_CODE (ScreenServer,screen_server, SCREEN_TYPE_ADMIN_SKELETON,
                         G_ADD_PRIVATE (ScreenServer) G_IMPLEMENT_INTERFACE (
                         SCREEN_TYPE_ADMIN, screen_server_screen_admin_iface_init));

gboolean
recorder_is_recording (ScreenServer *ss)
{
  g_return_val_if_fail (SCREEN_IS_SERVER (ss), FALSE);

  return ss->priv->state == RECORDER_STATE_RECORDING;
}

static void avi_format_call_back (ScreenServer *ss, int i)
{
    ss->priv->mux = gst_element_factory_make (video_formats[i].type, "muxer");
    gst_bin_add (GST_BIN (ss->priv->pipeline), ss->priv->mux);
}

static void vp8enc_format_call_back (ScreenServer *ss, int i)
{
    ss->priv->videnc = gst_element_factory_make ("vp8enc", "video_encoder");
    g_object_set (ss->priv->videnc, "cpu-used", 2, NULL);
    g_object_set (ss->priv->videnc, "end-usage", 0, NULL);
    g_object_set (ss->priv->videnc, "target-bitrate", 800000000, NULL);
    g_object_set (ss->priv->videnc, "static-threshold", 1000, NULL);
    g_object_set (ss->priv->videnc, "token-partitions", 2, NULL);
    g_object_set (ss->priv->videnc, "max-quantizer", 30, NULL);
    g_object_set (ss->priv->videnc, "threads", ss->priv->cpu_count -1, NULL);

    ss->priv->mux = gst_element_factory_make ("webmmux", "muxer");
    gst_bin_add (GST_BIN (ss->priv->pipeline), ss->priv->videnc);
    gst_bin_add (GST_BIN (ss->priv->pipeline), ss->priv->mux);
}

static void mp4_format_call_back (ScreenServer *ss, int i)
{
    ss->priv->videnc = gst_element_factory_make ("x264enc", "video_encoder");
    g_object_set (ss->priv->videnc, "pass", 4, NULL);
    g_object_set (ss->priv->videnc, "quantizer", 15, NULL);
    g_object_set (ss->priv->videnc, "threads", ss->priv->cpu_count -1, NULL);

    ss->priv->mux = gst_element_factory_make ("mp4mux", "muxer");
    g_object_set (ss->priv->mux, "faststart", 1, NULL);
    g_object_set (ss->priv->mux, "streamable", 1, NULL);
    gst_bin_add (GST_BIN (ss->priv->pipeline), ss->priv->videnc);
    gst_bin_add (GST_BIN (ss->priv->pipeline), ss->priv->mux);
}

static void mkv_format_call_back (ScreenServer *ss, int i)
{
    ss->priv->videnc = gst_element_factory_make ("x264enc", "video_encoder");

    ss->priv->mux = gst_element_factory_make ("matroskamux", "muxer");
    gst_bin_add (GST_BIN (ss->priv->pipeline), ss->priv->videnc);
    gst_bin_add (GST_BIN (ss->priv->pipeline), ss->priv->mux);
}

static void get_full_screen_info (ScreenServer *ss)
{
    GdkDisplay  *display;
    GdkMonitor  *monitor;
    int          num;
    int          scale;
    GdkRectangle rect;

    ss->priv->win_hash = g_hash_table_new (NULL, NULL);
    ss->priv->cpu_count = get_nprocs_conf ();

    display = gdk_display_get_default ();
    num = gdk_display_get_n_monitors (display);
    monitor = gdk_display_get_primary_monitor (display);
    scale = gdk_monitor_get_scale_factor (monitor);
    if (num > 1)
    {
        screen_message_dialog (_("Detection screen monitor"),
                               INFOR,
                               _("It is detected that the system has multiple screen monitors. At present, it only supports recording the primary monitor desktop"));
    }
    gdk_monitor_get_geometry (monitor, &rect);
    g_hash_table_insert (ss->priv->win_hash, "x", GINT_TO_POINTER(rect.x));
    g_hash_table_insert (ss->priv->win_hash, "y", GINT_TO_POINTER(rect.y));
    g_hash_table_insert (ss->priv->win_hash, "width", GINT_TO_POINTER(rect.width * scale));
    g_hash_table_insert (ss->priv->win_hash, "height", GINT_TO_POINTER(rect.height * scale));

}

static void cb_message (GstBus *bus, GstMessage *msg, gpointer data)
{
    ScreenServer *ss = SCREEN_SERVER (data);

    switch (GST_MESSAGE_TYPE (msg))
    {
        case GST_MESSAGE_ERROR:
        {
            GError *err;
            gchar *debug;

            gst_message_parse_error (msg, &err, &debug);
            g_warning ("Error: %s\n", err->message);
            g_error_free (err);
            g_free (debug);
            gst_element_set_state (ss->priv->pipeline, GST_STATE_NULL);
            break;
        }
        case GST_MESSAGE_EOS:
            gst_element_set_state (ss->priv->pipeline, GST_STATE_NULL);
            break;
        default:
            break;
    }
}

static void screen_server_init (ScreenServer *ss)
{
    GstBus *bus;
    ss->priv = screen_server_get_instance_private (ss);

    get_full_screen_info (ss);

    ss->priv->state = RECORDER_STATE_CLOSED;
    ss->priv->pipeline = gst_pipeline_new ("screen-pipeline");
    ss->priv->filter = gst_element_factory_make ("capsfilter", "vid_filter");
    ss->priv->videoconvert = gst_element_factory_make ("videoconvert", "videoconvert");
    ss->priv->videorate =    gst_element_factory_make ("videorate", "video_rate");
    ss->priv->in_queue = gst_element_factory_make ("queue", "queue_v1");
    ss->priv->out_queue = gst_element_factory_make ("queue", "queue_v2");
    ss->priv->sink = gst_element_factory_make ("filesink", "sink");
    ss->priv->file_queue = gst_element_factory_make ("queue", "queue_file");
    gst_bin_add_many (GST_BIN (ss->priv->pipeline),
                      ss->priv->filter,
                      ss->priv->videoconvert,
                      ss->priv->videorate,
                      ss->priv->in_queue,
                      ss->priv->out_queue,
                      ss->priv->sink,
                      ss->priv->file_queue,
                      NULL);

    bus = gst_element_get_bus (ss->priv->pipeline);
    gst_bus_add_signal_watch (bus);
    g_signal_connect (bus, "message", G_CALLBACK (cb_message), ss);

}

static void screen_server_finalize (GObject *object)
{
    ScreenServer *ss;

    ss = SCREEN_SERVER (object);
    g_hash_table_destroy (ss->priv->win_hash);
    if (ss->priv->file_name != NULL)
        g_free (ss->priv->file_name);
    if (ss->priv->pipeline != NULL)
    {
        gst_object_unref (ss->priv->pipeline);
        gst_element_send_event (ss->priv->pipeline, gst_event_new_eos());
    }
}

static void get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
    switch (prop_id)
    {
        case PROP_MANAGE_VERSION:
            g_value_set_string (value, VERSION);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
    switch (prop_id)
    {
        case PROP_MANAGE_VERSION:
            g_assert_not_reached ();
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void screen_server_class_init (ScreenServerClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = screen_server_finalize;
    object_class->get_property = get_property;
    object_class->set_property = set_property;

    g_object_class_override_property (object_class,
                                      PROP_MANAGE_VERSION,
                                      "daemon-version");
}

ScreenServer *screen_server_new(void)
{
    ScreenServer *ss = NULL;

    ss = g_object_new (SCREEN_TYPE_SERVER, NULL);

    return ss;
}

gboolean register_screen_server (ScreenServer *ss, GError **error)
{
    ss->priv->connection = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, error);
    if (ss->priv->connection == NULL)
    {
        return FALSE;
    }

    if (!g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (ss),
                                           ss->priv->connection,
                                          "/org/screen/admin",
                                           error))
    {
        return FALSE;
    }

    return TRUE;
}

static void setup_video_sources (ScreenServer *ss,
                                 gulong        xid,
                                 int           startx,
                                 int           starty,
                                 int           endx,
                                 int           endy,
                                 int           framerate,
                                 const char   *video_format)
{
    g_autofree char *text = NULL;
    GstCaps    *video_caps;
    guint i = 0;

    if (gst_bin_get_by_name (GST_BIN (ss->priv->pipeline), "video_src") != NULL)
    {
        gst_bin_remove (GST_BIN (ss->priv->pipeline), ss->priv->videosrc);
    }
    /* Property value has been modified during continuous recording */
    ss->priv->videosrc = gst_element_factory_make ("ximagesrc", "video_src");
    if (xid > 0)
    {
        g_object_set (ss->priv->videosrc, "xid", xid, NULL);
    }
    else
    {
        g_object_set (ss->priv->videosrc, "startx", startx, NULL);
        g_object_set (ss->priv->videosrc, "starty", starty, NULL);
        g_object_set (ss->priv->videosrc, "endx", endx, NULL);
        g_object_set (ss->priv->videosrc, "endy", endy, NULL);
        g_object_set (ss->priv->videosrc, "use-damage", FALSE, NULL);
    }

    text = g_strdup_printf ("video/x-raw, framerate=%d/1", framerate);
    video_caps = gst_caps_from_string (text);
    g_object_set (ss->priv->filter, "caps", video_caps, NULL);

    if (gst_bin_get_by_name (GST_BIN (ss->priv->pipeline), "video_encoder") != NULL)
    {
        gst_bin_remove (GST_BIN (ss->priv->pipeline), ss->priv->videnc);
    }
    if (gst_bin_get_by_name (GST_BIN (ss->priv->pipeline), "muxer") != NULL)
    {
        gst_bin_remove (GST_BIN (ss->priv->pipeline), ss->priv->mux);
    }
    gst_bin_add (GST_BIN (ss->priv->pipeline), ss->priv->videosrc);
    while (i < count)
    {
        if (g_strcmp0 (video_format, video_formats[i].format) == 0)
        {
            video_formats[i].func (ss, i);
        }
        i++;
    }
    if (i > count)
    {
        video_formats[0].func (ss,0);
    }
}

static void setup_links (ScreenServer *ss, gboolean is_raw)
{
    gst_element_link (ss->priv->videosrc, ss->priv->in_queue);
    gst_element_link (ss->priv->in_queue, ss->priv->videorate);
    gst_element_link (ss->priv->videorate, ss->priv->filter);
    gst_element_link (ss->priv->filter, ss->priv->videoconvert);
    if (is_raw)
        gst_element_link (ss->priv->videoconvert, ss->priv->out_queue);
    else
    {
        gst_element_link (ss->priv->videoconvert, ss->priv->videnc);
        gst_element_link (ss->priv->videnc,ss->priv-> out_queue);
    }
    gst_element_link (ss->priv->out_queue, ss->priv->mux);
    gst_element_link (ss->priv->mux, ss->priv->file_queue);
    gst_element_link (ss->priv->file_queue, ss->priv->sink);
}

static gboolean screencast_area (ScreenAdmin *object,
                                 GDBusMth    *invocation,
                                 GVariant    *area,
                                 const gchar *file_name,
                                 gint         framerate,
                                 gboolean     draw_cursor,
                                 const gchar *video_format)
{
    int startx = 0, starty = 0, endx = 0, endy = 0;
    gboolean is_raw = FALSE;

    ScreenServer *ss = SCREEN_SERVER (object);

    g_return_val_if_fail (ss->priv->state != RECORDER_STATE_RECORDING, FALSE);
    ss->priv->state = RECORDER_STATE_RECORDING;
    ss->priv->file_name = g_strdup (file_name);

    g_variant_lookup (area, "x", "n", &startx);
    g_variant_lookup (area, "y", "n", &starty);
    g_variant_lookup (area, "width", "n", &endx);
    g_variant_lookup (area, "height", "n", &endy);

    if (g_strcmp0 (video_format, "RAW (AVI)") == 0)
        is_raw = TRUE;

    setup_video_sources (ss, 0, startx, starty, endx+startx, endy+starty, framerate, video_format);
    g_object_set (ss->priv->videosrc, "show-pointer", draw_cursor, NULL);
    g_object_set (ss->priv->sink, "location", file_name, NULL);
    setup_links (ss, is_raw);
    gst_element_set_state (ss->priv->pipeline, GST_STATE_PLAYING);
    screen_admin_complete_screencast_area (object,invocation,TRUE);

    return TRUE;
}

static gboolean screencast_xid  (ScreenAdmin *object,
                                 GDBusMth    *invocation,
                                 guint        xid,
                                 const gchar *file_name,
                                 gint         framerate,
                                 gboolean     draw_cursor,
                                 const gchar *video_format)
{
    gboolean is_raw = FALSE;

    ScreenServer *ss = SCREEN_SERVER (object);
    g_return_val_if_fail (ss->priv->state != RECORDER_STATE_RECORDING, FALSE);

    ss->priv->state = RECORDER_STATE_RECORDING;
    if (g_strcmp0 (video_format, "RAW (AVI)") == 0)
        is_raw = TRUE;

    setup_video_sources (ss, xid, 0, 0, 0, 0, framerate, video_format);
    g_object_set (ss->priv->videosrc, "show-pointer", draw_cursor, NULL);
    g_object_set (ss->priv->sink, "location", file_name, NULL);
    setup_links (ss, is_raw);
    gst_element_set_state (ss->priv->pipeline, GST_STATE_PLAYING);

    screen_admin_complete_screencast_xid (object, invocation, TRUE);

    return TRUE;
}

static gboolean screencast_full (ScreenAdmin *object,
                                 GDBusMth    *invocation,
                                 const gchar *file_name,
                                 gint         framerate,
                                 gboolean     draw_cursor,
                                 const gchar *video_format)
{
    int startx, starty, width, height, endx, endy;
    gboolean is_raw = FALSE;
    ScreenServer *ss = SCREEN_SERVER (object);

    g_return_val_if_fail (ss->priv->state != RECORDER_STATE_RECORDING, FALSE);
    ss->priv->state = RECORDER_STATE_RECORDING;
    ss->priv->file_name = g_strdup (file_name);

    startx = GPOINTER_TO_INT (g_hash_table_lookup (ss->priv->win_hash, "x"));
    starty = GPOINTER_TO_INT (g_hash_table_lookup (ss->priv->win_hash, "y"));
    width = GPOINTER_TO_INT (g_hash_table_lookup (ss->priv->win_hash, "width"));
    height = GPOINTER_TO_INT (g_hash_table_lookup (ss->priv->win_hash, "height"));
    endx = startx + width - 1;
    endy = starty + height - 1;

    setup_video_sources (ss, 0, startx, starty, endx, endy, framerate,video_format);
    g_object_set (ss->priv->videosrc, "show-pointer", draw_cursor, NULL);
    g_object_set (ss->priv->sink, "location", file_name, NULL);
    if (g_strcmp0 (video_format, "RAW (AVI)") == 0)
        is_raw = TRUE;
    setup_links (ss, is_raw);
    gst_element_set_state (ss->priv->pipeline, GST_STATE_PLAYING);
    screen_admin_complete_screencast_full (object,invocation,TRUE);

    return TRUE;
}

static gboolean screencast_pause (ScreenAdmin *object,
                                  GDBusMth    *invocation)
{
    ScreenServer *ss = SCREEN_SERVER (object);
    g_return_val_if_fail (ss->priv->state != RECORDER_STATE_CLOSED, FALSE);

    gst_element_set_state (ss->priv->pipeline, GST_STATE_PAUSED);
    ss->priv->state = RECORDER_STATE_PAUSE;
    screen_admin_complete_screencast_pause (object, invocation, TRUE);
    return TRUE;

}

static gboolean screencast_unpause (ScreenAdmin *object,
                                    GDBusMth    *invocation)
{
    ScreenServer *ss = SCREEN_SERVER (object);
    g_return_val_if_fail (ss->priv->state == RECORDER_STATE_PAUSE, FALSE);

    ss->priv->state = RECORDER_STATE_RECORDING;
    gst_element_set_state (ss->priv->pipeline, GST_STATE_PLAYING);
    screen_admin_complete_screencast_unpause (object, invocation, TRUE);
    return TRUE;

}

static gboolean screencast_stop (ScreenAdmin *object,
                                 GDBusMth    *invocation)
{
    ScreenServer *ss = SCREEN_SERVER (object);
    g_return_val_if_fail (ss->priv->state != RECORDER_STATE_CLOSED, FALSE);

    ss->priv->state = RECORDER_STATE_CLOSED;
    gst_element_send_event (ss->priv->pipeline, gst_event_new_eos());

    screen_admin_complete_screencast_stop (object, invocation, TRUE);
    return TRUE;
}

static const gchar *screencast_get_daemon_version (ScreenAdmin *object)
{
    return VERSION;
}

static void screen_server_screen_admin_iface_init (ScreenAdminIface *iface)
{
    iface->handle_screencast_area    = screencast_area;
    iface->handle_screencast_full    = screencast_full;
    iface->handle_screencast_xid     = screencast_xid;
    iface->handle_screencast_pause   = screencast_pause;
    iface->handle_screencast_stop    = screencast_stop;
    iface->handle_screencast_unpause = screencast_unpause;
    iface->get_daemon_version =        screencast_get_daemon_version;
}
