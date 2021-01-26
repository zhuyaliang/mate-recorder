/*************************************************************************
  File Name: screen-stop.h

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

  Created Time: 2020年12月14日 星期一 09时32分04秒
 ************************************************************************/

#ifndef __SCREEN_STOP_H__
#define __SCREEN_STOP_H__

#include <gio/gio.h>
#include <gtk/gtk.h>
#include <libintl.h>
#include <locale.h>
#include <glib/gi18n.h>

G_BEGIN_DECLS

#define SCREEN_TYPE_STOP             (screen_stop_get_type ())
#define SCREEN_STOP(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), SCREEN_TYPE_STOP, ScreenStop))
#define SCREEN_IS_STOP(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SCREEN_TYPE_STOP))

typedef struct _ScreenStop          ScreenStop;
typedef struct _ScreenStopClass     ScreenStopClass;
typedef struct _ScreenStopPrivate   ScreenStopPrivate;

struct _ScreenStop
{
    GtkFrame parent;

    /*< private > */
    ScreenStopPrivate *priv;
};

struct _ScreenStopClass
{
    GtkFrameClass parent_class;
};

typedef enum
{
    STOP_BY_TIME = 0,
    STOP_BY_SIZE,
    STOP_BY_UNLIMITED,
}stop_type;

GType           screen_stop_get_type        (void);

GtkWidget      *screen_stop_new             (const char  *title);

int             screen_stop_get_stop_time   (ScreenStop *stop);

int             screen_stop_get_stop_size   (ScreenStop *stop);

stop_type       screen_stop_get_stop_mode   (ScreenStop *stop);

guint           start_screen_stop_monitor   (ScreenStop *stop);

G_END_DECLS

#endif
