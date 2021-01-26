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
                                               
  Created Time: 2021年01月08日 星期五 14时09分21秒
 ************************************************************************/

#include <stdio.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <libintl.h>
#include <locale.h>
#include <glib-unix.h>
#include <gst/gst.h>

#include "screen-server.h"

#define NAME_TO_CLAIM            "org.admin.screen"
#define GETTEXT_PACKAGE          "service"
#define LUNAR_CALENDAR_LOCALEDIR "/usr/share/locale"

static gboolean signal_quit (gpointer data)
{
    exit (0);
    return FALSE;
}

static void acquired_call_back (GDBusConnection *Connection,
                                const gchar     *name,
                                gpointer         data)
{
    ScreenServer *ss;
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
}

static void name_lost_call_back (GDBusConnection *connection,
                                 const gchar     *name,
                                 gpointer         data)
{
    g_warning("dbus name lost call back !!!!");
    exit (0);
}

int
main (int argc, char **argv)
{
    guint id;

    bindtextdomain (GETTEXT_PACKAGE,LUNAR_CALENDAR_LOCALEDIR);
    textdomain (GETTEXT_PACKAGE);
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


    g_unix_signal_add (SIGINT,  signal_quit, NULL);
    g_unix_signal_add (SIGTERM, signal_quit, NULL);

    gtk_main ();

    return 0;
}
