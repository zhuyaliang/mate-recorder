/*************************************************************************
  File Name: screen-list.h
  
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
                                               
  Created Time: 2021年02月04日 星期一 17时38分43秒
 ************************************************************************/

#ifndef __SCREEN_LIST__
#define __SCREEN_LIST__

#include <glib-object.h>
 
G_BEGIN_DECLS

#define SCREEN_TYPE_LIST         (screen_list_get_type ())
#define SCREEN_LIST(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), SCREEN_TYPE_LIST, ScreenList))
#define SCREEN_LIST_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), SCREEN_TYPE_LIST, ScreenListClass))
#define SCREEN_IS_LIST(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), SCREEN_TYPE_LIST))
#define SCREEN_LIST_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), SCREEN_TYPE_LIST, ScreenListClass))

typedef struct _ScreenList        ScreenList;
typedef struct _ScreenListClass   ScreenListClass;
typedef struct _ScreenListPrivate ScreenListPrivate;

struct _ScreenList {
    GtkDialog           parent_instance;
    ScreenListPrivate  *priv;
};

struct _ScreenListClass {
    GtkDialogClass parent_class;
};

GType         screen_list_get_type         (void) G_GNUC_CONST;

GtkWidget    *screen_list_new              (void);

gulong        screen_list_get_window_xid   (ScreenList *list);

void          screnn_set_window_activate   (ScreenList *list);
G_END_DECLS

#endif
