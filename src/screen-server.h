/*************************************************************************
  File Name: screen-server.h

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

  Created Time: 2021年01月08日 星期五 14时29分13秒
 ************************************************************************/

#ifndef __SCREEN_SERVER__
#define __SCREEN_SERVER__

#include <gst/gst.h>
#include "screen-generated.h"
G_BEGIN_DECLS

#define SCREEN_TYPE_SERVER         (screen_server_get_type ())
#define SCREEN_SERVER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), SCREEN_TYPE_SERVER, ScreenServer))
#define SCREEN_SERVER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), SCREEN_TYPE_SERVER, ScreenServerClass))
#define SCREEN_IS_SERVER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), SCREEN_TYPE_SERVER))
#define SCREEN_IS_SERVER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), SCREEN_TYPE_SERVER))
#define SCREEN_SERVER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), SCREEN_TYPE_SERVER, ScreenServerClass))

typedef struct _ScreenServer        ScreenServer;
typedef struct _ScreenServerClass   ScreenServerClass;
typedef struct _ScreenServerPrivate ScreenServerPrivate;

struct _ScreenServer
{
    ScreenAdminSkeleton   parent_instance;
    ScreenServerPrivate  *priv;
};

struct _ScreenServerClass
{
    ScreenAdminSkeletonClass parent_class;
};

GType           screen_server_get_type                (void) G_GNUC_CONST;

ScreenServer   *screen_server_new                     (void);

gboolean        register_screen_server                (ScreenServer *ss,
                                                       GError      **error);

gboolean        recorder_is_recording                 (ScreenServer *ss);
#endif
