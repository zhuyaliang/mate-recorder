/*************************************************************************
  File Name: screen-frame.c

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

  Created Time: 2020年12月14日 星期一 09时30分29秒
 ************************************************************************/

#include "screen-style.h"

struct _ScreenStylePrivate
{
    gboolean   show_cursor;
    int        framerate;
};

enum
{
    PROP_0,
    PROP_SHOW_CURSOR,
    PROP_FRAMERATE,
};

G_DEFINE_TYPE_WITH_PRIVATE (ScreenStyle, screen_style, GTK_TYPE_FRAME)

static void
screen_style_dispose (GObject *object)
{
//    ScreenStyle *style;
}

static void
screen_style_get_property (GObject    *object,
                           guint       param_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
    ScreenStyle *style = SCREEN_STYLE (object);

    switch (param_id)
    {
        case PROP_SHOW_CURSOR:
            g_value_set_boolean (value, style->priv->show_cursor);
            break;
        case PROP_FRAMERATE:
            g_value_set_int (value, style->priv->framerate);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
            break;
    }
}

static void
screen_style_set_property (GObject      *object,
                           guint         param_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
    ScreenStyle *style = SCREEN_STYLE (object);

    switch (param_id)
    {
        case PROP_SHOW_CURSOR:
            style->priv->show_cursor = g_value_get_boolean (value);
            break;
        case PROP_FRAMERATE:
            style->priv->framerate = g_value_get_int (value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
            break;
    }
}

static void
screen_style_init (ScreenStyle *style)
{

    GtkWidget *table;
    GtkWidget *label;
    GtkWidget *check_button;
    GtkWidget *spin;

    style->priv = screen_style_get_instance_private (style);
    style->priv->framerate = 15;
    table = gtk_grid_new();
    gtk_container_add (GTK_CONTAINER (style), table);
    gtk_grid_set_row_spacing(GTK_GRID(table), 10);
    gtk_grid_set_column_spacing(GTK_GRID(table), 10);
    gtk_grid_set_column_homogeneous(GTK_GRID(table), TRUE);

    label = gtk_label_new(_("show cursor when screencast"));
    gtk_widget_set_halign (label, GTK_ALIGN_START);
    gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
    gtk_grid_attach(GTK_GRID(table), label, 0, 0, 1, 1);

    check_button = gtk_check_button_new_with_mnemonic (_("Show cursor"));
    gtk_grid_attach(GTK_GRID(table), check_button, 1, 0, 1, 1);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_button), TRUE);
    style->priv->show_cursor = TRUE;
    g_object_bind_property (check_button, "active", style, "show_cursor", 0);

    label = gtk_label_new (_("framerate"));
    gtk_widget_set_halign (label, GTK_ALIGN_START);
    gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
    gtk_grid_attach(GTK_GRID(table), label, 0, 1, 1, 1);

    spin = gtk_spin_button_new_with_range (15, 60, 1);
    g_object_bind_property (spin, "value", style, "framerate", 0);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin), 15);
    gtk_grid_attach(GTK_GRID(table), spin, 1, 1, 1, 1);
}

static void
screen_style_class_init (ScreenStyleClass *style_class)
{
    GObjectClass *gobject_class;

    gobject_class = G_OBJECT_CLASS (style_class);
    gobject_class->set_property = screen_style_set_property;
    gobject_class->get_property = screen_style_get_property;

    gobject_class->dispose      = screen_style_dispose;
    g_object_class_install_property (
            gobject_class,
            PROP_SHOW_CURSOR,
            g_param_spec_boolean (
                    "show-cursor",
                    "Show Cursor",
                    "Is the cursor displayed when recording the screen",
                    TRUE,
                    G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (
            gobject_class,
            PROP_FRAMERATE,
            g_param_spec_int (
                    "framerate",
                    "Framerate",
                    "The frame rate at which the screen is recorded",
                    15,60,15,
                    G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

GtkWidget *
screen_style_new (const char *title)
{
    ScreenStyle *style = NULL;
    GtkWidget   *label;
    char        *text;

    style = g_object_new (SCREEN_TYPE_STYLE, NULL);
    gtk_frame_set_label (GTK_FRAME (style),"");
    text =  g_markup_printf_escaped("<span color = \'grey\' size=\"%s\" weight='bold'>%s</span>","large",title);
    label = gtk_frame_get_label_widget (GTK_FRAME (style));
    gtk_label_set_markup (GTK_LABEL (label),text);

    g_free (text);

    return GTK_WIDGET (style);
}

gboolean screen_style_get_show_cursor (ScreenStyle *style)
{
    return style->priv->show_cursor;
}

int screen_style_get_framerate (ScreenStyle *style)
{
    return style->priv->framerate;
}
