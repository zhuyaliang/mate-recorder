/*************************************************************************
  File Name: screen-window.h

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

  Created Time: 2020年12月15日 星期二 11时15分27秒
 ************************************************************************/
#ifndef __SCREEN_WINDOW__
#define __SCREEN_WINDOW__

#include <gtk/gtk.h>

#include "screen-server.h"

#define SCREEN_TYPE_WINDOW         (screen_window_get_type ())
#define SCREEN_WINDOW(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), SCREEN_TYPE_WINDOW, ScreenWindow))
#define SCREEN_WINDOW_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), SCREEN_TYPE_WINDOW, ScreenWindowClass))
#define SCREEN_IS_WINDOW(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), SCREEN_TYPE_WINDOW))
#define SCREEN_WINDOW_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), SCREEN_TYPE_WINDOW, ScreenWindowClass))

typedef struct _ScreenWindow        ScreenWindow;
typedef struct _ScreenWindowClass   ScreenWindowClass;
typedef struct _ScreenWindowPrivate ScreenWindowPrivate;

struct _ScreenWindow {
    GtkWindow             parent_instance;
    ScreenWindowPrivate  *priv;
};

struct _ScreenWindowClass {
    GtkWindowClass parent_class;
};
typedef enum
{
    ERROR = 0,
    WARING,
    INFOR,
    QUESTION
}MsgType;
GType         screen_window_get_type         (void) G_GNUC_CONST;

GtkWidget    *screen_window_new              (void);

void          destroy_screen_window          (ScreenWindow *ss);

int           screen_message_dialog          (const char   *title,
                                              const char   *msg,
                                              MsgType       type);
#endif
