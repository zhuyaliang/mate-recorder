/*************************************************************************
  File Name: screen-save.h

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

#ifndef __SCREEN_SAVE_H__
#define __SCREEN_SAVE_H__

#include <gio/gio.h>
#include <gtk/gtk.h>
#include <libintl.h>
#include <locale.h>
#include <glib/gi18n.h>

G_BEGIN_DECLS

#define SCREEN_TYPE_SAVE             (screen_save_get_type ())
#define SCREEN_SAVE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), SCREEN_TYPE_SAVE, ScreenSave))
#define SCREEN_IS_SAVE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SCREEN_TYPE_SAVE))

typedef struct _ScreenSave          ScreenSave;
typedef struct _ScreenSaveClass     ScreenSaveClass;
typedef struct _ScreenSavePrivate   ScreenSavePrivate;

struct _ScreenSave
{
    GtkFrame parent;

    /*< private > */
    ScreenSavePrivate *priv;
};

struct _ScreenSaveClass
{
    GtkFrameClass parent_class;
};

GType           screen_save_get_type           (void);

GtkWidget      *screen_save_new                (const char  *title);

char           *screen_save_get_folder_name    (ScreenSave *save);

char           *screen_save_get_video_format   (ScreenSave *save);

char           *screen_save_get_file_name      (ScreenSave *save);

void            screen_save_update_file_name   (ScreenSave *save);
G_END_DECLS

#endif
