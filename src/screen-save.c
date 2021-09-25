/*************************************************************************
  File Name: screen-save.c

  Copyright (C) 2020  zhuyaliang https://github.com/zhuyaliang/
  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.

  Created Time: 2020年12月14日 星期一 09时30分29秒
 ************************************************************************/

#include "screen-save.h"

#define  H264_PLUG                "/usr/share/gstreamer-1.0/presets/GstX264Enc.prs"
#define  TMP_MOVIES               "/tmp"

struct _ScreenSavePrivate
{
    char *folder_name;
    char *file_name;
    char *video_format;

    GHashTable *hash_table;
};

enum
{
    PROP_0,
    PROP_FILE_NAME,
    PROP_VIDEO_FORMAT,
};

enum
{
    FORMAT_CHANGED,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };
G_DEFINE_TYPE_WITH_PRIVATE (ScreenSave, screen_save, GTK_TYPE_FRAME)

static void screen_save_set_folder_name (ScreenSave *save,const char *folder_name)
{
    save->priv->folder_name = g_strdup (folder_name);
}

static void save_folder_changed_cb (GtkFileChooser *chooser,
                                    gpointer        user_data)
{
    ScreenSave *save = SCREEN_SAVE (user_data);
    gchar *folder_name;

    folder_name = gtk_file_chooser_get_uri (chooser);
    screen_save_set_folder_name (save, g_filename_from_uri (folder_name, NULL, NULL));
}

static void
screen_save_dispose (GObject *object)
{
    ScreenSave *save = SCREEN_SAVE (object);

    if (save->priv->folder_name != NULL)
    {
        g_free (save->priv->folder_name);
        save->priv->folder_name = NULL;
    }
    if (save->priv->file_name != NULL)
    {
        g_free (save->priv->file_name);
        save->priv->file_name = NULL;
    }
    if (save->priv->video_format != NULL)
    {
        g_free (save->priv->video_format);
        save->priv->video_format = NULL;
    }
    if (save->priv->hash_table)
    {
        g_hash_table_destroy (save->priv->hash_table);
        save->priv->hash_table = NULL;
    }
}

static void
screen_save_get_property (GObject    *object,
                          guint       param_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
    ScreenSave *save = SCREEN_SAVE (object);

    switch (param_id)
    {
        case PROP_FILE_NAME:
            g_value_set_string (value, save->priv->file_name);
            break;
        case PROP_VIDEO_FORMAT:
            g_value_set_string (value, save->priv->video_format);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
            break;
    }
}

static void
screen_save_set_property (GObject      *object,
                          guint         param_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
    ScreenSave *save = SCREEN_SAVE (object);

    switch (param_id)
    {
        case PROP_FILE_NAME:
            save->priv->file_name = g_value_dup_string (value);
            break;
        case PROP_VIDEO_FORMAT:
            save->priv->video_format = g_value_dup_string (value);
            g_signal_emit (save, signals[FORMAT_CHANGED], 0);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
            break;
    }
}

static char *get_screen_save_format_suffix (ScreenSave *save)
{
    char *suffix;
    g_return_val_if_fail (SCREEN_IS_SAVE (save), NULL);
    g_return_val_if_fail (save->priv->hash_table != NULL, NULL);

    suffix = g_hash_table_lookup (save->priv->hash_table, save->priv->video_format);

    return suffix;
}

static char *get_screen_save_file_name (char *suffix)
{
    char *time3;
    int i = 0;
    g_autofree gchar *time1,*time2 = NULL;
    g_autoptr(GDateTime) date_time = NULL;
    char *text = _("Screen video");

    date_time = g_date_time_new_now_local ();
    time1 = g_date_time_format (date_time, ("%x"));
    time2 = g_date_time_format (date_time, ("%X"));
    time3 = g_strdup_printf ("%s%s%s.%s", time1, text, time2, suffix);

    while (time3[i] != '\0')
    {
        if (G_IS_DIR_SEPARATOR (time3[i]))
        {
            time3[i] = '-';
        }
        i++;
    }
    return time3;
}

static GHashTable *create_hash_table (void)
{
    GHashTable *hash;

    hash = g_hash_table_new (g_str_hash, g_str_equal);
    g_hash_table_insert (hash, "RAW (AVI)", "avi");
    g_hash_table_insert (hash, "VP8 (WEBM)", "webm");
    g_hash_table_insert (hash, "H264 (MP4)", "mp4");
    g_hash_table_insert (hash, "H264 (MKV)", "mkv");

    return hash;
}

static GtkWidget *
create_video_format_combox (void)
{
    const char *video_formats[] = {"VP8 (WEBM)", "RAW (AVI)", "H264 (MP4)", "H264 (MKV)", NULL};
    GtkWidget  *combox;
    gboolean    sensitive = TRUE;
    int         i = 0;

    if (access (H264_PLUG, F_OK) !=0)
    {
        sensitive = FALSE;
    }
    combox = gtk_combo_box_text_new ();
    while (video_formats[i] != NULL)
    {
        gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (combox), video_formats[i], video_formats[i]);
        i++;
        if (!sensitive && i > 1)
            break;
    }

    return combox;
}

static void
record_format_changed (ScreenSave  *save,
                       gpointer     user_data)
{
    screen_save_update_file_name (save);
}

static void
screen_save_init (ScreenSave *save)
{
    GtkWidget  *label;
    GtkWidget  *table;
    GtkWidget  *combox;
    GtkWidget  *picker;
    GtkWidget  *entry;
    g_autofree  gchar *time;
    const char *video;
    char       *suffix;

    save->priv = screen_save_get_instance_private (save);

    save->priv->hash_table = create_hash_table ();

    table = gtk_grid_new ();
    gtk_container_add (GTK_CONTAINER (save), table);
    gtk_grid_set_row_spacing (GTK_GRID (table), 10);
    gtk_grid_set_column_spacing (GTK_GRID (table), 10);
    gtk_grid_set_column_homogeneous (GTK_GRID (table), TRUE);

    label = gtk_label_new (_("Video Foramt"));
    gtk_widget_set_halign (label, GTK_ALIGN_START);
    gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
    gtk_grid_attach (GTK_GRID (table), label, 0, 0, 1, 1);

    combox = create_video_format_combox ();
    g_object_bind_property (combox,
                           "active-id",
                           save,
                           "video-format",
                           G_BINDING_BIDIRECTIONAL);
    gtk_combo_box_set_active_id (GTK_COMBO_BOX (combox), "VP8 (WEBM)");
    gtk_grid_attach (GTK_GRID (table), combox, 1, 0, 1, 1);

    label = gtk_label_new (_("Folder"));
    gtk_widget_set_halign (label, GTK_ALIGN_START);
    gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
    gtk_grid_attach (GTK_GRID (table), label, 0, 1, 1, 1);

    picker = gtk_file_chooser_button_new ("Pick a Folder",
                                          GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);

    g_signal_connect (picker,
                     "selection-changed",
                      G_CALLBACK (save_folder_changed_cb),
                      save);

    video = g_get_user_special_dir (G_USER_DIRECTORY_VIDEOS);
    if (video == NULL)
    {
        gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (picker), TMP_MOVIES);
        screen_save_set_folder_name (save, TMP_MOVIES);
    }
    else
    {
        gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (picker), video);
        screen_save_set_folder_name (save, video);
    }
    gtk_grid_attach (GTK_GRID (table), picker, 1, 1, 1, 1);

    label = gtk_label_new (_("FileName"));
    gtk_widget_set_halign (label, GTK_ALIGN_START);
    gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
    gtk_grid_attach (GTK_GRID (table), label, 0, 2, 1, 1);

    entry = gtk_entry_new ();
    g_object_bind_property (entry, "text", save, "file-name", G_BINDING_BIDIRECTIONAL);

    suffix = get_screen_save_format_suffix (save);
    time = get_screen_save_file_name (suffix);
    gtk_entry_set_text (GTK_ENTRY (entry), time);
    gtk_grid_attach (GTK_GRID (table), entry, 1, 2, 1, 1);
}

static void
screen_save_class_init (ScreenSaveClass *save_class)
{
    GObjectClass *gobject_class;

    gobject_class = G_OBJECT_CLASS (save_class);
    gobject_class->set_property = screen_save_set_property;
    gobject_class->get_property = screen_save_get_property;

    gobject_class->dispose = screen_save_dispose;

    g_object_class_install_property (
            gobject_class,
            PROP_FILE_NAME,
            g_param_spec_string (
                    "file-name",
                    "File Name",
                    "The name of the file where the recording results are saved",
                     NULL,
                     G_PARAM_READWRITE));

    g_object_class_install_property (
            gobject_class,
            PROP_VIDEO_FORMAT,
            g_param_spec_string (
                    "video-format",
                    "VIDEO FORMAT",
                    "Format of recorded video",
                     "VP8 (WEBM)",
                     G_PARAM_READWRITE));

    signals [FORMAT_CHANGED] =
         g_signal_new ("format-changed",
                       G_TYPE_FROM_CLASS (save_class),
                       G_SIGNAL_RUN_LAST,
                       0,
                       NULL, NULL,
                       g_cclosure_marshal_VOID__VOID,
                       G_TYPE_NONE, 0);

}

GtkWidget *
screen_save_new (const char *title)
{
    ScreenSave  *save = NULL;
    GtkWidget   *label;
    char        *text;

    save = g_object_new (SCREEN_TYPE_SAVE, NULL);
    g_signal_connect (save,
                     "format-changed",
                      G_CALLBACK (record_format_changed),
                      NULL);
    gtk_frame_set_label (GTK_FRAME (save), "");
    text =  g_markup_printf_escaped ("<span color = \'grey\' size=\"%s\" weight='bold'>%s</span>","large",title);
    label = gtk_frame_get_label_widget (GTK_FRAME (save));
    gtk_label_set_markup (GTK_LABEL (label),text);

    g_free (text);

    return GTK_WIDGET (save);
}

char *screen_save_get_folder_name (ScreenSave *save)
{
    return save->priv->folder_name;
}

char *screen_save_get_video_format (ScreenSave *save)
{
    return save->priv->video_format;
}

char *screen_save_get_file_name (ScreenSave *save)
{
    int i = 0;
    if (strlen (save->priv->file_name) == 0)
        screen_save_update_file_name (save);

    while (save->priv->file_name[i] != '\0')
    {
        if (G_IS_DIR_SEPARATOR (save->priv->file_name[i]))
        {
            save->priv->file_name[i] = '-';
        }
        i++;
    }
    return save->priv->file_name;
}

void screen_save_update_file_name (ScreenSave *save)
{
    g_autofree gchar *time;
    char             *suffix;

    suffix = get_screen_save_format_suffix (save);
    time = get_screen_save_file_name (suffix);

    g_object_set (save, "file-name", time, NULL);
}
