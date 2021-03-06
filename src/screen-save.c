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

struct _ScreenSavePrivate
{
    char *folder_name;
    char *file_name;
    char *video_format;
};
enum
{
    PROP_0,
    PROP_FILE_NAME,
    PROP_VIDEO_FORMAT,
};

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
    screen_save_set_folder_name (save,g_filename_from_uri (folder_name,NULL,NULL));
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
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
            break;
    }
}

static char *get_screen_save_file_name (void)
{
    char *time3;
    int i = 0;
    g_autofree gchar *time1,*time2 = NULL;
    g_autoptr(GDateTime) date_time = NULL;
    char *text = _("Screen video");

    date_time = g_date_time_new_now_local ();
    time1 = g_date_time_format (date_time, ("%x"));
    time2 = g_date_time_format (date_time, ("%X"));
    time3 = g_strdup_printf ("%s%s%s.webm",time1,text,time2);

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

static void
screen_save_init (ScreenSave *save)
{
    GtkWidget *label;
    GtkWidget *table;
    GtkWidget *picker;
    GtkWidget *entry;
    g_autofree gchar *time;
    const char *video;
    save->priv = screen_save_get_instance_private (save);

    table = gtk_grid_new();
    gtk_container_add (GTK_CONTAINER (save), table);
    gtk_grid_set_row_spacing(GTK_GRID(table), 10);
    gtk_grid_set_column_spacing(GTK_GRID(table), 10);
    gtk_grid_set_column_homogeneous(GTK_GRID(table), TRUE);

    label = gtk_label_new (_("Folder"));
    gtk_widget_set_halign (label, GTK_ALIGN_START);
    gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
    gtk_grid_attach (GTK_GRID (table), label, 0, 0, 1, 1);

    picker = gtk_file_chooser_button_new ("Pick a Folder",
                                          GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);

    g_signal_connect (picker,
                     "selection-changed",
                      G_CALLBACK (save_folder_changed_cb),
                      save);

    video = g_get_user_special_dir (G_USER_DIRECTORY_VIDEOS);
    gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (picker), video);
    screen_save_set_folder_name (save,video);
    gtk_grid_attach (GTK_GRID (table), picker, 1, 0, 1, 1);

    label = gtk_label_new (_("FileName"));
    gtk_widget_set_halign (label, GTK_ALIGN_START);
    gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
    gtk_grid_attach (GTK_GRID (table), label, 0, 1, 1, 1);

    entry = gtk_entry_new ();
    g_object_bind_property (entry, "text", save, "file_name", G_BINDING_BIDIRECTIONAL);
    time = get_screen_save_file_name ();
    gtk_entry_set_text (GTK_ENTRY (entry), time);
    gtk_grid_attach (GTK_GRID (table), entry, 1, 1, 1, 1);
}

static void
screen_save_class_init (ScreenSaveClass *save_class)
{
    GObjectClass *gobject_class;

    gobject_class = G_OBJECT_CLASS (save_class);
    gobject_class->set_property = screen_save_set_property;
    gobject_class->get_property = screen_save_get_property;

    gobject_class->dispose      = screen_save_dispose;

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
            PROP_FILE_NAME,
            g_param_spec_string (
                    "video-format",
                    "VIDEO FORMAT",
                    "Format of recorded video",
                     "VP8 (WEBM)",
                     G_PARAM_READWRITE));

}

GtkWidget *
screen_save_new (const char *title)
{
    ScreenSave  *save = NULL;
    GtkWidget   *label;
    char        *text;

    save = g_object_new (SCREEN_TYPE_SAVE, NULL);
    gtk_frame_set_label (GTK_FRAME (save),"");
    text =  g_markup_printf_escaped("<span color = \'grey\' size=\"%s\" weight='bold'>%s</span>","large",title);
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

    time = get_screen_save_file_name ();

    g_object_set (save, "file-name", time, NULL);
}
