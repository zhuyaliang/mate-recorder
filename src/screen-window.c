/*************************************************************************
  File Name: screen-window.c

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

  Created Time: 2020年12月15日 星期二 11时17分28秒
 ************************************************************************/
#include <libappindicator/app-indicator.h>
#include <libnotify/notify.h>

#include "screen-window.h"
#include "screen-style.h"
#include "screen-stop.h"
#include "screen-save.h"
#include "screen-count.h"
#include "screen-area.h"
#include "screen-list.h"
#include "screen-generated.h"

#define SCREEN_NAME         "org.screen.admin"
#define SCREEN_PATH         "/org/screen/admin"

#define MSGFORMAT    "<span foreground='red'font_desc='13'>%s </span>"

typedef enum
{
    FULL_SCREEN,
    AREA_SCREEN,
    XID_SCREEN
}RecordMode;

struct _ScreenWindowPrivate 
{
    GDBusProxy   *proxy;
    AppIndicator *indicator;
    NotifyNotification *notify;

    GtkWidget  *style;
    GtkWidget  *stop;
    GtkWidget  *save;
    GtkWidget  *count;
    GtkWidget  *area;
    GtkWidget  *list;
    GtkWidget  *settings_item;
    GtkWidget  *start_item;
    GtkWidget  *stop_item;
    GtkWidget  *quit_item;
    GtkWidget  *skip_item;

    GtkWidget  *button_full;
    GtkWidget  *button_area;
    GtkWidget  *button_xid;
    gboolean    is_start;
    char       *save_path;

    guint       second;
    RecordMode  mode;
    guint       minute;
    gboolean    show_label;
    guint       tray_timeout_id;
    gulong      sig1_id,sig2_id;
    gulong      sig1_xid_id,sig2_xid_id;
};

G_DEFINE_TYPE_WITH_PRIVATE (ScreenWindow, screen_window, GTK_TYPE_WINDOW)

static void stop_screencast (ScreenWindow *screenwin);

static NotifyNotification *get_notification (void)
{
    NotifyNotification *notify;

    notify_init ("Mate-Recorder");
    notify = notify_notification_new ("mate-recorder",
                                      _("Screen  ready"),
                                      "emblem-default");
    notify_notification_set_urgency (notify, NOTIFY_URGENCY_LOW);
    notify_notification_set_timeout (notify, 2000);

    return notify;
}

static void open_recorder_dir_callback (NotifyNotification *notification,
                                        const char         *action,
                                        const char         *path)
{
    char   *dir;
    char   *argv[3];
    GError *error = NULL;

    dir = g_path_get_dirname (path);

    argv[0] = "/usr/bin/caja";
    argv[1] = dir;
    argv[2] = NULL;

    if (!g_spawn_async (NULL, argv, NULL, G_SPAWN_DO_NOT_REAP_CHILD, NULL, NULL, NULL, &error))
    {
        screen_message_dialog (_("Open Save Dir"), ERROR, error->message);
    }
}

static void set_notify_action (NotifyNotification *notify,
                               const char         *path)
{
    if (g_find_program_in_path ("caja") == NULL)
    {
        return;
    }
    notify_notification_add_action (notify,
                                   "action",
                                   _("Enter recording directory"),
                                   (NotifyActionCallback) open_recorder_dir_callback,
                                   path,
                                   NULL);

}

static void screen_admin_update_notification (NotifyNotification *notify,
                                              const char         *summary,
                                              const char         *body,
                                              const char         *icon,
                                              const char         *dir_path)
{
    if (notify == NULL)
        return;
    notify_notification_update (notify, summary, body, icon);
    notify_notification_clear_actions (notify);
    if (dir_path != NULL)
    {
        set_notify_action (notify, dir_path);
    }
    notify_notification_show (notify, NULL);
}

static void
screen_start_item_cb (GtkMenuItem *item, gpointer user_data)
{
    ScreenWindow *screenwin = SCREEN_WINDOW (user_data);
    ScreenSave   *save  = SCREEN_SAVE  (screenwin->priv->save);

    gtk_widget_set_sensitive (screenwin->priv->stop_item,  TRUE);
    gtk_widget_set_sensitive (screenwin->priv->start_item, FALSE);

    gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (screenwin->priv->button_full), TRUE);
    gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (screenwin->priv->button_area), FALSE);
    gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (screenwin->priv->button_xid), FALSE);
    gtk_widget_show (GTK_WIDGET (screenwin));
    screen_save_update_file_name (save);
}

static void update_tray_time (ScreenWindow *screenwin)
{
    gchar * percentstr;

    g_source_remove (screenwin->priv->tray_timeout_id);
    screenwin->priv->tray_timeout_id = 0;

    screenwin->priv->minute = 0;
    screenwin->priv->second = 0;
    
    if (screenwin->priv->show_label)
    {
        percentstr = g_strdup_printf("%02u:%02u", screenwin->priv->minute, screenwin->priv->second);
        app_indicator_set_label (screenwin->priv->indicator, percentstr, "100%");
        g_free(percentstr);
    }
    else
        app_indicator_set_label (screenwin->priv->indicator, NULL, NULL);
}

static void
screen_stop_item_cb (GtkMenuItem *item, gpointer user_data)
{
    ScreenWindow *screenwin = SCREEN_WINDOW (user_data);
    ScreenList *list = SCREEN_LIST (screenwin->priv->list);

    gtk_widget_set_sensitive (screenwin->priv->stop_item, FALSE);
    if (screenwin->priv->list != NULL)
        screnn_set_window_activate (list);
    stop_screencast (screenwin);
    update_tray_time (screenwin);
    gtk_widget_set_sensitive (screenwin->priv->start_item, TRUE);
}

static void
screen_quit_item_cb (GtkMenuItem *item, gpointer user_data)
{
    ScreenWindow *screenwin = SCREEN_WINDOW (user_data);

    screen_admin_update_notification (screenwin->priv->notify,
                                      _("Close application"),
                                      _("Application closed successfully"),
                                      "face-worried",
                                      NULL);

    destroy_screen_window (screenwin);
}

static void
screen_skip_item_cb (GtkMenuItem *item, gpointer user_data)
{
    ScreenWindow *screenwin = SCREEN_WINDOW (user_data);

    gtk_widget_set_sensitive (screenwin->priv->skip_item, FALSE);
    screen_stop_count_down (SCREEN_COUNT (screenwin->priv->count));
}

static void
screen_time_item_cb (GtkCheckMenuItem *item, gpointer user_data)
{
    ScreenWindow *screenwin = SCREEN_WINDOW (user_data);

    screenwin->priv->show_label = gtk_check_menu_item_get_active (item);
    if (!screenwin->priv->show_label)
        app_indicator_set_label (screenwin->priv->indicator, NULL, NULL);
    else if (screenwin->priv->minute == 0 && screenwin->priv->second == 0)
    {
        app_indicator_set_label (screenwin->priv->indicator, "00:00", "100%");
    }
}
static GtkWidget *get_menu_button (ScreenWindow *screenwin)
{
    GtkWidget *menu;
    GtkWidget *separator_item;
    GtkWidget *time_item;
    
    menu = gtk_menu_new ();

    screenwin->priv->settings_item = gtk_menu_item_new_with_label (_("Settings"));
    gtk_widget_set_sensitive (screenwin->priv->settings_item, TRUE);

    screenwin->priv->start_item = gtk_menu_item_new_with_label (_("Start new recording"));
    g_signal_connect (screenwin->priv->start_item,
                     "activate",
                      G_CALLBACK (screen_start_item_cb),
                      screenwin);
    gtk_widget_set_sensitive (screenwin->priv->start_item, FALSE);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), screenwin->priv->start_item);

    screenwin->priv->stop_item = gtk_menu_item_new_with_label (_("Stop recording"));
    g_signal_connect (screenwin->priv->stop_item,
                     "activate",
                      G_CALLBACK (screen_stop_item_cb),
                      screenwin);
    gtk_widget_set_sensitive (screenwin->priv->stop_item, FALSE);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), screenwin->priv->stop_item);

    screenwin->priv->skip_item = gtk_menu_item_new_with_label (_("Skip countdown"));
    gtk_widget_set_sensitive (screenwin->priv->skip_item, FALSE);
    g_signal_connect (screenwin->priv->skip_item,
                     "activate",
                      G_CALLBACK (screen_skip_item_cb),
                      screenwin);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), screenwin->priv->skip_item);

    separator_item = gtk_separator_menu_item_new ();
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), separator_item);

    time_item = gtk_check_menu_item_new_with_label (_("Show Time"));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), time_item);
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (time_item), TRUE);
    g_signal_connect (time_item,
                     "toggled",
                      G_CALLBACK (screen_time_item_cb),
                      screenwin);

    separator_item = gtk_separator_menu_item_new ();
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), separator_item);

    screenwin->priv->quit_item = gtk_menu_item_new_with_label (_("Quit recording"));
    g_signal_connect (screenwin->priv->quit_item,
                     "activate",
                      G_CALLBACK (screen_quit_item_cb),
                      screenwin);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), screenwin->priv->quit_item);

    gtk_widget_show_all (menu);

    return menu;
}

static void create_tray_indicator (ScreenWindow *screenwin)
{
    GtkWidget *menu;

    menu = get_menu_button (screenwin);

    screenwin->priv->indicator = app_indicator_new ("mate-recorder-menu",
                                                    "screen-start",
                                                     APP_INDICATOR_CATEGORY_APPLICATION_STATUS);
    app_indicator_set_attention_icon_full(screenwin->priv->indicator, "screen-start", "Local Attention Icon");
    app_indicator_set_status (screenwin->priv->indicator, APP_INDICATOR_STATUS_ATTENTION);
    app_indicator_set_label (screenwin->priv->indicator, "00:00", "100%");

    app_indicator_set_title (screenwin->priv->indicator, "mate-recorder-menu");
    app_indicator_set_menu (screenwin->priv->indicator, GTK_MENU(menu));
}

static GVariantBuilder *get_screencast_area (gint x, gint y, gint w, gint h)
{
    GVariantBuilder *builder;

    builder = g_variant_builder_new (G_VARIANT_TYPE ("a{sn}"));
    g_variant_builder_add (builder, "{sn}", "x",      x);
    g_variant_builder_add (builder, "{sn}", "y",      y);
    g_variant_builder_add (builder, "{sn}", "width",  w);
    g_variant_builder_add (builder, "{sn}", "height", h);

    return builder;
}

static char *get_screencast_save_path (ScreenSave *save)
{
    char     *save_path;
    char     *folder_name;
    char     *file_name;
    char     *new_name;
    int       num = 1;

    folder_name = screen_save_get_folder_name (save);
    file_name = screen_save_get_file_name (save);

    save_path = g_build_filename (folder_name, file_name, NULL);
    if (!g_file_test (save_path, G_FILE_TEST_EXISTS))
        return save_path;

    while (TRUE)
    {
        new_name = g_strdup_printf ("%s-%d", file_name, num);
        save_path = g_build_filename (folder_name, new_name, NULL);
        g_free (new_name);
        if (!g_file_test (save_path, G_FILE_TEST_EXISTS))
            return save_path;
        num++;
    }
}

static void
start_screencast_done (GObject      *source_object,
                       GAsyncResult *res,
                       gpointer      data)
{
    g_autoptr(GError) error = NULL;
    GVariant    *result;

    result = g_dbus_proxy_call_finish (G_DBUS_PROXY (source_object), res, &error);

    if (result == NULL)
    {
        if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        {
            screen_message_dialog (_("start screen recording"), ERROR, error->message);
        }
    }
}

static void start_screencast (ScreenWindow *screenwin)
{
    GVariantBuilder *variant;
    ScreenSave      *save;
    ScreenStyle     *style;
    ScreenList      *list;
    char            *video_format;
    gboolean         show_cursor;
    uint             framerate;
    gulong           xid;
    gint32 x, y, h ,w;

    save = SCREEN_SAVE (screenwin->priv->save);
    style = SCREEN_STYLE (screenwin->priv->style);

    show_cursor = screen_style_get_show_cursor (style);
    framerate = screen_style_get_framerate (style);
    screenwin->priv->save_path = get_screencast_save_path (save);
    video_format = screen_save_get_video_format (save);

    if (screenwin->priv->mode == FULL_SCREEN)
    {
        g_dbus_proxy_call (screenwin->priv->proxy,
                          "ScreencastFull",
                           g_variant_new ("(sibs)", screenwin->priv->save_path, framerate, show_cursor, video_format),
                           G_DBUS_CALL_FLAGS_NONE,
                           -1,
                           NULL,
                           (GAsyncReadyCallback)start_screencast_done,
                           screenwin);
    }
    else if (screenwin->priv->mode == AREA_SCREEN)
    {
        x = screen_area_get_startx (SCREEN_AREA (screenwin->priv->area));
        y = screen_area_get_starty (SCREEN_AREA (screenwin->priv->area));
        h = screen_area_get_height (SCREEN_AREA (screenwin->priv->area));
        w = screen_area_get_width  (SCREEN_AREA (screenwin->priv->area));
        variant = get_screencast_area (x, y, w, h);

        g_dbus_proxy_call (screenwin->priv->proxy,
                          "ScreencastArea",
                           g_variant_new ("(a{sn}sibs)",variant, screenwin->priv->save_path, framerate, show_cursor, video_format),
                           G_DBUS_CALL_FLAGS_NONE,
                           -1,
                           NULL,
                           (GAsyncReadyCallback)start_screencast_done,
                           screenwin);
    }
    else if (screenwin->priv->mode == XID_SCREEN)
    {
        list = SCREEN_LIST (screenwin->priv->list);
        xid = screen_list_get_window_xid (list);
        screnn_set_window_activate (list);

        g_dbus_proxy_call (screenwin->priv->proxy,
                          "ScreencastXid",
                           g_variant_new ("(usibs)",xid, screenwin->priv->save_path, framerate, show_cursor, video_format),
                           G_DBUS_CALL_FLAGS_NONE,
                           -1,
                           NULL,
                           (GAsyncReadyCallback)start_screencast_done,
                           screenwin);
    }

}

static void
stop_screencast_done (GObject      *source_object,
                      GAsyncResult *res,
                      gpointer      data)
{
    g_autoptr(GError) error = NULL;
    GVariant    *result;
    ScreenWindow *screenwin = SCREEN_WINDOW (data);

    result = g_dbus_proxy_call_finish (G_DBUS_PROXY (source_object), res, &error);

    if (result == NULL)
    {
        if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        {
            g_printerr ("Error setting OSD's visibility: %s\n", error->message);
        }
    }

    screen_admin_update_notification (screenwin->priv->notify,
                                      _("End of recording"),
                                      screenwin->priv->save_path,
                                      "face-cool",
                                      screenwin->priv->save_path);

}

static void stop_screencast (ScreenWindow *screenwin)
{
    g_dbus_proxy_call(screenwin->priv->proxy,
                     "ScreencastStop",
                      g_variant_new ("()"),
                      G_DBUS_CALL_FLAGS_NONE,
                      -1,
                      NULL,
                      (GAsyncReadyCallback) stop_screencast_done,
                      screenwin);
}

static gboolean
screen_time_changed (gpointer user_data)
{
    ScreenWindowPrivate *priv = (ScreenWindowPrivate*) user_data;

    priv->second++;
    if (priv->second >= 60)
    {
        priv->second = 0;
        priv->minute++;
    }
    if (priv->show_label)
    {
        gchar * percentstr = g_strdup_printf("%02u:%02u", priv->minute, priv->second);
        app_indicator_set_label (priv->indicator, percentstr, "100%");
        g_free(percentstr);
    }
    else
    {
        app_indicator_set_label (priv->indicator, NULL, NULL);
    }
    return TRUE;
}

static void create_indicator_time (ScreenWindowPrivate *priv)
{
    if (priv->show_label)
        app_indicator_set_label (priv->indicator, "00:01", "100%");
    priv->second = 1;
    priv->tray_timeout_id  = g_timeout_add_seconds(1, screen_time_changed, priv);
}

static void countdown_finished_cb (ScreenCount *count, gpointer user_data)
{
    ScreenWindow    *screenwin = SCREEN_WINDOW (user_data);

    start_screencast (screenwin);
    start_screen_stop_monitor (SCREEN_STOP (screenwin->priv->stop));
    create_indicator_time (screenwin->priv);
    gtk_widget_set_sensitive (screenwin->priv->stop_item, TRUE);
    gtk_widget_set_sensitive (screenwin->priv->skip_item, FALSE);
}

static void screencast_button_cb (GtkWidget *button, gpointer user_data)
{
    ScreenWindow *screenwin = SCREEN_WINDOW (user_data);
    ScreenCount  *count = SCREEN_COUNT (screenwin->priv->count);

    gtk_widget_hide (GTK_WIDGET (screenwin));
    gtk_widget_set_sensitive (screenwin->priv->skip_item, TRUE);
    screen_start_count_down (count);
}

static void countdown_stop_cb (ScreenCount *count, gpointer user_data)
{
    ScreenWindow    *screenwin = SCREEN_WINDOW (user_data);

    gtk_widget_set_sensitive (screenwin->priv->stop_item, FALSE);
    stop_screencast (screenwin);
    update_tray_time (screenwin);
    gtk_widget_set_sensitive (screenwin->priv->start_item, TRUE);
}

static GtkWidget *create_start_and_stop_screencast (ScreenWindow *screenwin)
{
    GtkWidget *hbox;
    GtkWidget *button;

    ScreenCount  *count = SCREEN_COUNT (screenwin->priv->count);
    ScreenStop   *stop =  SCREEN_STOP  (screenwin->priv->stop);
    hbox = gtk_button_box_new (GTK_ORIENTATION_HORIZONTAL);
    button = gtk_button_new_with_label (_("Start Recording"));

    g_signal_connect (button,
                     "clicked",
                     (GCallback) screencast_button_cb,
                      screenwin);

    g_signal_connect (count,
                      "finished",
                     (GCallback) countdown_finished_cb,
                      screenwin);

    g_signal_connect (stop,
                      "stoped",
                     (GCallback) countdown_stop_cb,
                      screenwin);
    gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 12);

    return hbox;
}

static void
screean_area_select_cb (ScreenArea *area,
                        gpointer    user_data)
{
    ScreenWindow *screenwin = SCREEN_WINDOW (user_data);

    screenwin->priv->mode = AREA_SCREEN;
    gtk_widget_show (GTK_WIDGET (screenwin));
}

static void
screean_area_cancel_cb (ScreenArea *area,
                        gpointer    user_data)
{
    ScreenWindow *screenwin = SCREEN_WINDOW (user_data);
    gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (screenwin->priv->button_full), TRUE);
    gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (screenwin->priv->button_area), FALSE);
    screenwin->priv->mode = FULL_SCREEN;
    gtk_widget_show (GTK_WIDGET (screenwin));
}

static void
screen_area_mode_cb (GtkToggleToolButton *button,
                     gpointer             user_data)
{
    gboolean    active;
    GtkWidget  *area;
    const char *name;

    ScreenWindow *screenwin = SCREEN_WINDOW (user_data);

    active = gtk_toggle_tool_button_get_active(button);
    name = gtk_widget_get_name (GTK_WIDGET (button));

    if (g_strcmp0 (name, "Area") == 0 && active == TRUE)
    {
        gtk_widget_hide (GTK_WIDGET (screenwin));
        area = screen_area_new ();
        screenwin->priv->area = area;

        screenwin->priv->sig1_id = g_signal_connect (area,
                                              "selected",
                                              (GCallback) screean_area_select_cb,
                                               screenwin);

        screenwin->priv->sig2_id = g_signal_connect (area,
                                              "canceled",
                                              (GCallback) screean_area_cancel_cb,
                                               screenwin);

        gtk_widget_show_all (area);
        screenwin->priv->mode = AREA_SCREEN;
    }
    else if (g_strcmp0 (name, "Area") == 0 && active == FALSE)
    {
        screenwin->priv->mode = FULL_SCREEN;
        if (screenwin->priv->sig1_id > 0)
        {
            g_signal_handler_disconnect (screenwin->priv->area, screenwin->priv->sig1_id);
            g_signal_handler_disconnect (screenwin->priv->area, screenwin->priv->sig2_id);
            gtk_widget_destroy (screenwin->priv->area);
        }
    }
}

static void
screean_xid_select_cb (ScreenList *list,
                       gpointer    user_data)
{
    ScreenWindow *screenwin = SCREEN_WINDOW (user_data);

    screenwin->priv->mode = XID_SCREEN;
    gtk_widget_show (GTK_WIDGET (screenwin));
}

static void
screean_xid_cancel_cb (ScreenList *list,
                       gpointer    user_data)
{
    ScreenWindow *screenwin = SCREEN_WINDOW (user_data);
    gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (screenwin->priv->button_full), TRUE);
    gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (screenwin->priv->button_xid), FALSE);

    screenwin->priv->mode = FULL_SCREEN;
    gtk_widget_show (GTK_WIDGET (screenwin));
}

static void
screen_xid_mode_cb (GtkToggleToolButton *button,
                    gpointer             user_data)
{
    gboolean    active;
    GtkWidget  *list;
    const char *name;

    ScreenWindow *screenwin = SCREEN_WINDOW (user_data);

    active = gtk_toggle_tool_button_get_active(button);
    name = gtk_widget_get_name (GTK_WIDGET (button));

    if (g_strcmp0 (name, "Xid") == 0 && active == TRUE)
    {
        gtk_widget_hide (GTK_WIDGET (screenwin));

        list = screen_list_new ();
        screenwin->priv->list = list;

        screenwin->priv->sig1_xid_id = g_signal_connect (list,
                                                        "selected",
                                                        (GCallback) screean_xid_select_cb,
                                                         screenwin);

        screenwin->priv->sig2_xid_id = g_signal_connect (list,
                                                        "canceled",
                                                        (GCallback) screean_xid_cancel_cb,
                                                         screenwin);

        gtk_widget_show_all (list);
        screenwin->priv->mode = XID_SCREEN;
    }
    else if (g_strcmp0 (name, "Xid") == 0 && active == FALSE)
    {
        screenwin->priv->mode = FULL_SCREEN;
        if (screenwin->priv->sig1_xid_id > 0)
        {
            g_signal_handler_disconnect (screenwin->priv->list, screenwin->priv->sig1_xid_id);
            g_signal_handler_disconnect (screenwin->priv->list, screenwin->priv->sig2_xid_id);
            gtk_widget_destroy (screenwin->priv->list);
        }
    }
}
static void
screen_window_fill (ScreenWindow *screenwin)
{
    GtkWidget   *vbox;
    GtkWidget   *hbox;
    GtkWidget   *toolbar;
    GtkToolItem *item;
    GSList      *group;

    GtkWidget *frame_style,*frame_stop,*frame_save,*frame_count;

    vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_add (GTK_CONTAINER (screenwin), vbox);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);

    toolbar = gtk_toolbar_new ();
    gtk_widget_set_valign (toolbar, GTK_ALIGN_CENTER);
    gtk_widget_set_halign (toolbar, GTK_ALIGN_CENTER);
    gtk_box_pack_start (GTK_BOX (vbox), toolbar, FALSE, FALSE, 12);

    item = gtk_radio_tool_button_new (NULL);
    gtk_widget_set_name (GTK_WIDGET (item), "Full");
    gtk_tool_button_set_label (GTK_TOOL_BUTTON (item), _("Full Screen"));
    group = gtk_radio_tool_button_get_group (GTK_RADIO_TOOL_BUTTON (item));
    gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);
    screenwin->priv->button_full = GTK_WIDGET (item);

    item = gtk_radio_tool_button_new (group);
    gtk_widget_set_name (GTK_WIDGET (item), "Area");
    g_signal_connect (item,
                     "toggled",
                      G_CALLBACK (screen_area_mode_cb),
                      screenwin);

    gtk_tool_button_set_label (GTK_TOOL_BUTTON (item), _("Selection Area"));
    group = gtk_radio_tool_button_get_group (GTK_RADIO_TOOL_BUTTON (item));
    gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);
    screenwin->priv->button_area = GTK_WIDGET (item);

    item = gtk_radio_tool_button_new (group);
    gtk_widget_set_name (GTK_WIDGET (item), "Xid");
    g_signal_connect (item,
                     "toggled",
                      G_CALLBACK (screen_xid_mode_cb),
                      screenwin);
    gtk_tool_button_set_label (GTK_TOOL_BUTTON (item), _("Selection window"));
    gtk_toolbar_insert (GTK_TOOLBAR (toolbar), item, -1);
    screenwin->priv->button_xid = GTK_WIDGET (item);

    frame_style = screen_style_new (_("Recording screen settings"));
    gtk_box_pack_start (GTK_BOX (vbox), frame_style, FALSE, FALSE, 12);
    screenwin->priv->style = frame_style;

    frame_stop = screen_stop_new (_("Stop Mode"));
    gtk_box_pack_start (GTK_BOX (vbox), frame_stop, FALSE, FALSE, 12);
    screenwin->priv->stop = frame_stop;

    frame_save = screen_save_new (_("Save Mode"));
    gtk_box_pack_start (GTK_BOX (vbox), frame_save, FALSE, FALSE, 12);
    screenwin->priv->save = frame_save;

    frame_count = screen_count_new (_("count down"));
    gtk_box_pack_start (GTK_BOX (vbox), frame_count, FALSE, FALSE, 12);
    screenwin->priv->count = frame_count;

    hbox = create_start_and_stop_screencast (screenwin);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 12);

}
static GObject *
screen_window_constructor (GType                  type,
                           guint                  n_construct_properties,
                           GObjectConstructParam *construct_properties)
{
    GObject        *obj;
    ScreenWindow   *screenwin;

    obj = G_OBJECT_CLASS (screen_window_parent_class)->constructor (type,
                                      n_construct_properties,
                                      construct_properties);

    screenwin = SCREEN_WINDOW (obj);
    screen_window_fill (screenwin);
    screen_admin_update_notification (screenwin->priv->notify,
                                     _("Start application"),
                                     _("The recording program is ready. Please start recording"),
                                     "face-smile",
                                     NULL);

    return obj;
}
static void
screen_window_dispose (GObject *object)
{
    ScreenWindow *screenwin;

    screenwin = SCREEN_WINDOW (object);
    g_object_unref (screenwin->priv->proxy);

    if (screenwin->priv->save_path != NULL)
    {
        g_free (screenwin->priv->save_path);
    }
    if (screenwin->priv->tray_timeout_id != 0)
    {
        g_source_remove (screenwin->priv->tray_timeout_id);
        screenwin->priv->tray_timeout_id = 0;
    }
    G_OBJECT_CLASS (screen_window_parent_class)->dispose (object);
}

static void
screen_window_class_init (ScreenWindowClass *klass)
{
    GObjectClass   *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->constructor = screen_window_constructor;
    gobject_class->dispose = screen_window_dispose;
}
static void
screen_window_init (ScreenWindow *screenwin)
{
    GtkWindow  *window;

    screenwin->priv = screen_window_get_instance_private (screenwin);
    screenwin->priv->proxy = g_dbus_proxy_new_for_bus_sync (
                             G_BUS_TYPE_SESSION,
                             G_DBUS_PROXY_FLAGS_NONE,
                             NULL,
                             SCREEN_NAME,
                             SCREEN_PATH,
                             SCREEN_NAME,
                             NULL, NULL);

    window = GTK_WINDOW (screenwin);
    gtk_window_set_title (GTK_WINDOW (window), _("Mate Recording Screen"));
    gtk_window_set_resizable (GTK_WINDOW (window), FALSE);

    gtk_window_set_position (window, GTK_WIN_POS_CENTER);
    gtk_window_set_default_size (GTK_WINDOW (window),
                                 400, 400);
    screenwin->priv->show_label = TRUE;
    screenwin->priv->mode = FULL_SCREEN;
    screenwin->priv->list = NULL;
    create_tray_indicator (screenwin);
    screenwin->priv->notify = get_notification ();
}

GtkWidget *
screen_window_new (void)
{
    ScreenWindow *screenwin;

    screenwin = g_object_new (SCREEN_TYPE_WINDOW,
                             "type", GTK_WINDOW_TOPLEVEL,
                              NULL);

    if (screenwin->priv->proxy == NULL)
    {
        return NULL;
    }
    return GTK_WIDGET (screenwin);
}

void destroy_screen_window (ScreenWindow *screenwin)
{
    gtk_widget_destroy (screenwin->priv->style);
    gtk_widget_destroy (screenwin->priv->save);
    gtk_widget_destroy (screenwin->priv->stop);
    gtk_widget_destroy (screenwin->priv->count);
    gtk_widget_destroy (GTK_WIDGET (screenwin));
}

int screen_message_dialog(const char *title, MsgType type, const char *msg,...)
{
    GtkWidget *dialog = NULL;
    va_list    args;
    char      *message;
    int        ret;

    switch(type)
    {
        case ERROR:
        {
            dialog = gtk_message_dialog_new(NULL,
                                            GTK_DIALOG_DESTROY_WITH_PARENT,
                                            GTK_MESSAGE_ERROR,
                                            GTK_BUTTONS_OK,
                                            "%s",title);
            break;
        }
        case WARING:
        {
            dialog = gtk_message_dialog_new(NULL,
                                            GTK_DIALOG_DESTROY_WITH_PARENT,
                                            GTK_MESSAGE_WARNING,
                                            GTK_BUTTONS_OK,
                                            "%s",title);
            break;
        }
        case INFOR:
        {
            dialog = gtk_message_dialog_new(NULL,
                                            GTK_DIALOG_DESTROY_WITH_PARENT,
                                            GTK_MESSAGE_INFO,
                                            GTK_BUTTONS_OK,
                                            "%s",title);
            break;
        }
        case QUESTION:
        {
            dialog = gtk_message_dialog_new(NULL,
                                            GTK_DIALOG_DESTROY_WITH_PARENT,
                                            GTK_MESSAGE_QUESTION,
                                            GTK_BUTTONS_YES_NO,
                                            "%s",title);
            break;
        }
        default :
            break;

    }

    va_start(args, msg);
    message = g_strdup_vprintf (msg, args);
    va_end(args);

    gtk_message_dialog_format_secondary_markup(GTK_MESSAGE_DIALOG(dialog),
                                               MSGFORMAT,
                                               message);
    gtk_window_set_title(GTK_WINDOW(dialog),_("Message"));
    ret =  gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    g_free (message);

    return ret;
}
