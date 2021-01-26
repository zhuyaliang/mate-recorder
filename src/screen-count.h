/*************************************************************************
  File Name: screen-count.h
  
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

#ifndef __SCREEN_COUNT_H__
#define __SCREEN_COUNT_H__

#include <gio/gio.h>
#include <gtk/gtk.h>
#include <libintl.h>
#include <locale.h>
#include <glib/gi18n.h>

G_BEGIN_DECLS

#define SCREEN_TYPE_COUNT             (screen_count_get_type ())
#define SCREEN_COUNT(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), SCREEN_TYPE_COUNT, ScreenCount))
#define SCREEN_IS_COUNT(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SCREEN_TYPE_COUNT))

typedef struct _ScreenCount          ScreenCount;
typedef struct _ScreenCountClass     ScreenCountClass;
typedef struct _ScreenCountPrivate   ScreenCountPrivate;

struct _ScreenCount
{
    GtkFrame parent;

    /*< private > */
    ScreenCountPrivate *priv;
};

struct _ScreenCountClass
{
    GtkFrameClass parent_class;
};

GType           screen_count_get_type        (void);

GtkWidget      *screen_count_new             (const char  *title);

gboolean        screen_start_count_down      (ScreenCount *count);

void            screen_stop_count_down       (ScreenCount *count);
G_END_DECLS

#endif

