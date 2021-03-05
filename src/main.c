/*************************************************************************
  File Name: main.c

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

  Created Time: 2020年12月09日 星期三 17时40分39秒
 ************************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#include "screen-window.h"
#include "screen-stop.h"
#include "screen-save.h"
#include "screen-count.h"
#include "config.h"

#define  GDBUD_ERROR _("Error connecting screen-server gdbus interface. Please check whether screen-server is installed and running")
#define  NAME_TO_CLAIM            "org.screen.admin"
static void remove_lock_dir (void)
{
    DIR  *dir;
    struct dirent *ptr;
    char *file_path;

    dir = opendir(LOCKDIR);
    while ((ptr = readdir(dir)) != NULL)
    {
        if(strcmp(ptr->d_name,".")==0 || strcmp(ptr->d_name,"..")==0)
            continue;
        if(ptr->d_type == 8)
        {
            file_path = g_build_filename (LOCKDIR, ptr->d_name, NULL);
            remove (file_path);
            g_free (file_path);
        }
    }
    closedir(dir);
    remove (LOCKDIR);
}
static void app_quit(GtkWidget *object,
                     gpointer   user_data)
{
    if (access(LOCKDIR,F_OK) == 0)
    {
        remove_lock_dir ();
    }
    destroy_screen_window (SCREEN_WINDOW (object));
    exit (0);
}

static void create_pid_file (void)
{
    pid_t pid;
    int   fd;
    char *lock_file;

    pid = getpid();
    lock_file = g_strdup_printf ("%s/%d", LOCKDIR, pid);
    fd = creat (lock_file, 0777);
    g_free (lock_file);
    close (fd);
}
static gboolean check_process_already_running (void)
{
    DIR  *dir;
    pid_t pid;
    struct dirent *ptr;

    umask(0);

    if (access(LOCKDIR,F_OK) !=0)
    {
        mkdir (LOCKDIR, S_IRWXU|S_IRWXO|S_IRWXG);
        create_pid_file ();
        return FALSE;
    }
    else
    {
        dir = opendir(LOCKDIR);
        while ((ptr = readdir(dir)) != NULL)
        {
            if(strcmp(ptr->d_name,".")==0 || strcmp(ptr->d_name,"..")==0)
                continue;
            if(ptr->d_type == 8)
            {
                pid = atoi (ptr->d_name);
                if(kill(pid, 0) == 0)
                {
                    return TRUE;
                }
            }
        }
        closedir(dir);
    }
    create_pid_file ();
    return FALSE;
}

static void acquired_call_back (GDBusConnection *Connection,
                                const gchar     *name,
                                gpointer         data)
{
    ScreenServer *ss;
    GtkWidget    *window;
    GdkPixbuf    *pixbuf = NULL;
    GError       *error = NULL;

    ss = screen_server_new();
    if (ss == NULL)
    {
        g_warning ("Failed to initialize video source");
        exit (0);
    }
    
    if(register_screen_server (ss, &error) < 0)
    {
        g_warning ("register video source interface failed %s", error->message);
        exit (0);
    }
    window = screen_window_new ();
    if (window == NULL)
    {
       screen_message_dialog (_("Init GDbus"), GDBUD_ERROR, ERROR);
    }
    g_signal_connect (window,
                     "destroy",
                      G_CALLBACK (app_quit),
                      NULL);
    pixbuf = gdk_pixbuf_new_from_file(ICONPATH, NULL);
    gtk_window_set_icon(GTK_WINDOW(window), pixbuf);

    gtk_widget_show_all (window);
}

static void name_lost_call_back (GDBusConnection *connection,
                                 const gchar     *name,
                                 gpointer         data)
{
    screen_message_dialog (_("Init GDbus"), _("dbus name lost call back !!!!"), ERROR);
    exit (0);
}

int
main (int argc, char **argv)
{
    guint     id;

    bindtextdomain (GETTEXT_PACKAGE,LUNAR_CALENDAR_LOCALEDIR);
    textdomain (GETTEXT_PACKAGE);

    if (check_process_already_running ())
    {
        return 0;
    }

    gtk_init (&argc, &argv);
    gst_init (&argc, &argv);
    id = g_bus_own_name (G_BUS_TYPE_SESSION,
                         NAME_TO_CLAIM,
                         G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT,
                         acquired_call_back,
                         NULL,
                         name_lost_call_back,
                         NULL,
                         NULL);

    gtk_main ();
    g_bus_unown_name (id);
    return 0;
}
