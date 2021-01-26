/*************************************************************************
  File Name: screen-area.h
  
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
                                               
  Created Time: 2021年01月18日 星期一 10时07分43秒
 ************************************************************************/

#ifndef __SCREEN_AREA__
#define __SCREEN_AREA__

#include <glib-object.h>
 
G_BEGIN_DECLS

#define SCREEN_TYPE_AREA         (screen_area_get_type ())
#define SCREEN_AREA(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), SCREEN_TYPE_AREA, ScreenArea))
#define SCREEN_AREA_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), SCREEN_TYPE_AREA, ScreenAreaClass))
#define SCREEN_IS_AREA(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), SCREEN_TYPE_AREA))
#define SCREEN_AREA_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), SCREEN_TYPE_AREA, ScreenAreaClass))

typedef struct _ScreenArea        ScreenArea;
typedef struct _ScreenAreaClass   ScreenAreaClass;
typedef struct _ScreenAreaPrivate ScreenAreaPrivate;

struct _ScreenArea {
    GtkWindow           parent_instance;
    ScreenAreaPrivate  *priv;
};

struct _ScreenAreaClass {
    GtkWindowClass parent_class;
};

GType         screen_area_get_type         (void) G_GNUC_CONST;

GtkWidget    *screen_area_new              (void);

gint32        screen_area_get_height       (ScreenArea *area);

gint32        screen_area_get_width        (ScreenArea *area);

gint32        screen_area_get_startx       (ScreenArea *area);

gint32        screen_area_get_starty       (ScreenArea *area);

G_END_DECLS

#endif
