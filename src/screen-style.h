/*************************************************************************
  File Name: screen-frame.h

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

#ifndef __SCREEN_STYLE_H__
#define __SCREEN_STYLE_H__

#include <gio/gio.h>
#include <gtk/gtk.h>
#include <libintl.h>
#include <locale.h>
#include <glib/gi18n.h>

G_BEGIN_DECLS

#define SCREEN_TYPE_STYLE             (screen_style_get_type ())
#define SCREEN_STYLE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), SCREEN_TYPE_STYLE, ScreenStyle))
#define SCREEN_IS_STYLE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SCREEN_TYPE_STYLE))

typedef struct _ScreenStyle          ScreenStyle;
typedef struct _ScreenStyleClass     ScreenStyleClass;
typedef struct _ScreenStylePrivate   ScreenStylePrivate;

struct _ScreenStyle
{
    GtkFrame parent;
    
    /*< private > */
    ScreenStylePrivate *priv;
};

struct _ScreenStyleClass
{
    GtkFrameClass parent_class;
};

GType           screen_style_get_type        (void);

GtkWidget      *screen_style_new             (const char  *title);

gboolean        screen_style_get_show_cursor (ScreenStyle *style);

int             screen_style_get_framerate   (ScreenStyle *style);

G_END_DECLS

#endif

