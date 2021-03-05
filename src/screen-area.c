/* Copyright (C) 2020 zhuyaliang https://github.com/zhuyaliang/
 *
 * Ported from kazam 
 * Copyright (C) 2012 David Klasinc <bigwhale@lubica.net>
 *
 * This file is part of mate-recorder.
 *
 * mate-recorder is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * mate-recorder is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with mate-recorder.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <math.h>
#include <gio/gio.h>
#include <gtk/gtk.h>
#include <libintl.h>
#include <locale.h>
#include <glib/gi18n.h>

#include "screen-area.h"

enum
{
    SELECTED,
    CANCELED,
    LAST_SIGNAL
};
struct _ScreenAreaPrivate
{
    int startx;
    int starty;
    int endx;
    int endy;
    int g_startx;
    int g_starty;
    int g_endx;
    int g_endy;
    int height;
    int width;
    int resize_handle;
    int move_offsetx;
    int move_offsety;

    gboolean   compositing;
    GdkCursor *last_cursor;
    GdkDevice *device;
    GdkWindow *root_window;

};

static guint signals[LAST_SIGNAL] = { 0 };
G_DEFINE_TYPE_WITH_PRIVATE (ScreenArea, screen_area, GTK_TYPE_WINDOW)

const int HANDLE_CURSORS[]= {
    GDK_TOP_LEFT_CORNER,
    GDK_TOP_SIDE,
    GDK_TOP_RIGHT_CORNER,
    GDK_LEFT_SIDE,
    GDK_FLEUR,
    GDK_RIGHT_SIDE,
    GDK_BOTTOM_LEFT_CORNER,
    GDK_BOTTOM_SIDE,
    GDK_BOTTOM_RIGHT_CORNER
};
typedef enum
{
    HANDLE_TL = 0,
    HANDLE_TC,
    HANDLE_TR,
    HANDLE_CL,
    HANDLE_MOVE,
    HANDLE_CR,
    HANDLE_BL,
    HANDLE_BC,
    HANDLE_BR,
}Resize;

static int get_window_scaling (void)
{
    GdkDisplay   *ds;
    GdkMonitor   *monitor;
    gint          scale;

    ds = gdk_display_get_default ();
    monitor = gdk_display_get_primary_monitor (ds);
    scale = gdk_monitor_get_scale_factor (monitor);

    return scale;
}

static GdkRectangle get_rectangle_data (GdkDevice *device)
{
    GdkDisplay   *ds;
    GdkMonitor   *monitor;
    GdkRectangle  rect;

    ds = gdk_device_get_display (device);
    monitor = gdk_display_get_primary_monitor (ds);
    gdk_monitor_get_geometry (monitor, &rect);

    return rect;
}

static double in_circle (double center_x, double center_y, int radius, int x, int y)
{
    double dist;
    double x1, y1;

    x1 = pow ((center_x - x), 2);
    y1 = pow ((center_y - y), 2);
    dist = sqrt(x1 + y1);

    return dist <= radius;
}

static void restore_default_cursor (ScreenArea *area)
{
    gdk_window_set_cursor (area->priv->root_window,
                           area->priv->last_cursor);
}

static void swap (int *a, int *b)
{
    int tmp;

    if (*a > *b)
    {
        tmp = *a;
        *a = *b;
        *b = tmp;
    }
}

static void accept_area (ScreenArea *area)
{

    restore_default_cursor (area);

    swap (&area->priv->startx, &area->priv->endx);
    swap (&area->priv->g_startx, &area->priv->g_endx);
    swap (&area->priv->starty, &area->priv->endy);
    swap (&area->priv->g_starty, &area->priv->g_endy);

    if (area->priv->startx < 0)
        area->priv->startx = 0;

    if (area->priv->starty < 0)
        area->priv->starty = 0;

    area->priv->width  = ABS(area->priv->endx - area->priv->startx);
    area->priv->height = ABS(area->priv->endy - area->priv->starty);

//    g_print ("Selected coords: = g_startx = %d g_starty = %d g_endx = %d g_endy = %d\r\n",area->priv->g_startx, area->priv->g_starty, area->priv->g_endx, area->priv->g_endy);
}

static gboolean
cb_keypress_event (GtkWidget *widget,
                   GdkEvent  *event,
                   gpointer   user_data)
{
    guint16 keycode;

    ScreenArea *area = SCREEN_AREA (user_data);

    gdk_event_get_keycode (event, &keycode);
    if (keycode == 36 || keycode == 104)// Enter
    {
        restore_default_cursor (area);
        gtk_widget_hide (widget);
        accept_area (area);
        g_signal_emit (area, signals[SELECTED], 0);
    }
    else if (keycode == 9) //ESC
    {
        restore_default_cursor (area);
        gtk_widget_hide (widget);
        g_signal_emit (area, signals[CANCELED], 0);
    }

    return TRUE;
}
static void outline_text(cairo_t *cr, int w, int h, double size, const char *text, gboolean cmp)
{
    cairo_text_extents_t extents;
    double cx, cy;

    cairo_set_font_size (cr, size);
/*???????????????????????*/
    cairo_select_font_face (cr, "Ubuntu", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_text_extents (cr, text, &extents);
    cairo_set_line_width (cr, 2.0);

    cx = w/2 - extents.width/2;
    cy = h/2 - extents.height/2;

    if (cmp)
    {
        cairo_set_source_rgba(cr, 0.4, 0.4, 0.4, 1.0);
    }
    else
    {
        cairo_set_source_rgb (cr, 0.4, 0.4, 0.4);
    }
    cairo_move_to (cr, cx, cy);
    cairo_text_path (cr, text);
    cairo_stroke (cr);
    if (cmp)
    {
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
    }
    else
    {
        cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
    }
    cairo_move_to (cr, cx, cy);
    cairo_show_text (cr, text);
}

static gboolean
cb_draw (GtkWidget    *widget,
         cairo_t      *cairo,
         gpointer      user_data)
{
    int    w, h;
    double centerx, centery;
    double x, y;
    g_autofree char *size = NULL; 
    int    i = 0;

    cairo_pattern_t *grad;
    ScreenArea *area = SCREEN_AREA (user_data);

    gtk_window_get_size (GTK_WINDOW (area), &w, &h);
    if (area->priv->compositing)
    {
        cairo_set_source_rgba(cairo, 0.0, 0.0, 0.0, 0.45);
    }
    else
    {
        cairo_set_source_rgb (cairo, 0.5, 0.5, 0.5);
    }
    cairo_set_operator (cairo, CAIRO_OPERATOR_SOURCE);
    cairo_paint (cairo);
    cairo_set_line_width (cairo, 1.0);
    cairo_set_source_rgb (cairo, 1.0, 1.0, 1.0);
    cairo_rectangle (cairo,
                     area->priv->startx,
                     area->priv->starty,
                     area->priv->width,
                     area->priv->height);
    cairo_stroke (cairo);
    if (area->priv->compositing)
    {
        cairo_set_source_rgba(cairo, 0.0, 0.0, 0.0, 0.0);
    }
    else
    {
        cairo_set_source_rgb (cairo, 0.0, 0.0, 0.0);
    }
    cairo_rectangle (cairo,
                     area->priv->startx + 1,
                     area->priv->starty + 1,
                     area->priv->width  - 2,
                     area->priv->height - 2);
    cairo_fill (cairo);
    cairo_set_operator (cairo, CAIRO_OPERATOR_OVER);

    for (i = 0 ; i < 9; i++)
    {
        if (i == HANDLE_MOVE)
            continue;
        x = floor(i % 3) / 2;
        y = floor(i / 3) / 2;

        centerx = area->priv->startx + area->priv->width * x;
        centery = area->priv->starty + area->priv->height * y;
        grad = cairo_pattern_create_radial(centerx,
                                           centery,
                                           0,
                                           centerx,
                                           centery + 2,
                                           10);
        cairo_pattern_add_color_stop_rgba (grad, 0.6, 0.0, 0.0, 0.0, 0.6);
        cairo_pattern_add_color_stop_rgba (grad, 0.75, 0.0, 0.0, 0.0, 0.25);
        cairo_pattern_add_color_stop_rgba (grad, 1.0, 0.0, 0.0, 0.0, 0.0);
        cairo_arc (cairo, centerx, centery, 10, 0, 2 * M_PI);
        cairo_set_source (cairo, grad);
        cairo_fill (cairo);

        grad = cairo_pattern_create_linear (centerx, centery - 8, centerx, centery + 8);
        cairo_pattern_add_color_stop_rgb (grad, 0.0, 0.75, 0.75, 0.75);
        cairo_pattern_add_color_stop_rgb (grad, 0.75, 0.95, 0.95, 0.95);
        cairo_arc (cairo, centerx, centery, 8, 0, 2 * M_PI);
        cairo_set_source (cairo, grad);
        cairo_fill (cairo);

        cairo_set_source_rgb (cairo, 1.0, 1.0, 1.0);
        cairo_arc (cairo, centerx, centery, 8, 0, 2 * M_PI);
        cairo_stroke (cairo);
    }
    size = g_strdup_printf ("%d X %d",ABS(area->priv->width + 1), ABS(area->priv->height + 1));
    outline_text(cairo, w, h, 30, _("Select an area by clicking and dragging."), area->priv->compositing);
    outline_text(cairo, w, h + 50, 26, _("Press ENTER to confirm or ESC to cancel"), area->priv->compositing);
    outline_text(cairo, w, h + 100, 20, size, area->priv->compositing);
    cairo_set_operator (cairo, CAIRO_OPERATOR_SOURCE);

    return TRUE;
}
static gboolean
cb_draw_motion_notify_event (GtkWidget       *widget,
                             GdkEventMotion  *event,
                             gpointer         user_data)
{
    GdkWindow *window;
    double     x, y;
    int        sh, sw, x1, y1;
    int        ex, ey, sx, sy;
    int        i = 0;
    double     offsetx, offsety;
    GdkCursor *cursor;
    GdkDisplay   *ds;
    GdkRectangle  rect;
    GdkModifierType mask;
    gboolean cursor_changed = FALSE;

    ScreenArea *area = SCREEN_AREA (user_data);
    window = event->window;

    rect = get_rectangle_data (area->priv->device);
    ds = gdk_device_get_display (area->priv->device);
    gdk_window_get_device_position (window, area->priv->device, &x1, &y1, &mask);

    ex = event->x;
    ey = event->y;
    sx = rect.x;
    sy = rect.y;

    for (i = 0; i < 9; i++)
    {
        x = floor(i % 3) / 2;
        y = floor(i / 3) / 2;
        offsetx = area->priv->width * x;
        offsety = area->priv->height * y;

        if (area->priv->g_startx > area->priv->g_endx)
           offsetx *= -1;
        if (area->priv->g_starty > area->priv->g_endy)
           offsety *= -1;

        if (in_circle(MIN(area->priv->g_startx, area->priv->g_endx) + offsetx,
                      MIN(area->priv->g_starty, area->priv->g_endy) + offsety,
                      8, sx + ex, sy + ey))
        {
            cursor_changed = TRUE;
            cursor = gdk_cursor_new_for_display (ds, HANDLE_CURSORS[i]);
            gdk_window_set_cursor(area->priv->root_window, cursor);
            break;
        }
        cursor = gdk_cursor_new_for_display (ds, GDK_CROSSHAIR);
        gdk_window_set_cursor(area->priv->root_window, cursor);
    }
    if (!cursor_changed && \
         MIN(area->priv->startx, area->priv->endx) < ex  &&\
         ex < MAX(area->priv->startx, area->priv->endx) &&\
         MIN(area->priv->starty, area->priv->endy) < ey &&\
         ey < MAX(area->priv->starty, area->priv->endy))
    {
        cursor = gdk_cursor_new_for_display (ds, HANDLE_CURSORS[HANDLE_MOVE]);
        gdk_window_set_cursor(area->priv->root_window, cursor);

    }

    if (mask & GDK_BUTTON1_MASK)
    {
        if (area->priv->resize_handle == HANDLE_TL)
        {
            area->priv->startx = ex;
            area->priv->starty = ey;
            area->priv->g_startx = sx + ex;
            area->priv->g_starty = sy + ey;
        }
        else if(area->priv->resize_handle == HANDLE_TC)
        {
            area->priv->starty = ey;
            area->priv->g_starty = sy + ey;

        }
        else if (area->priv->resize_handle == HANDLE_TR)
        {
            area->priv->endx = ex;
            area->priv->starty = ey;
            area->priv->g_endx = sx + ex;
            area->priv->g_starty = sy + ey;
        }
        else if (area->priv->resize_handle == HANDLE_CL)
        {
            area->priv->startx = ex;
            area->priv->g_startx = sx + ex;
        }
        else if (area->priv->resize_handle == HANDLE_CR)
        {
            area->priv->endx = ex;
            area->priv->g_endx = sx + ex;
        }
        else if (area->priv->resize_handle == HANDLE_BL)
        {
            area->priv->startx = ex;
            area->priv->endy = ey;
            area->priv->g_startx = sx + ex;
            area->priv->g_endy = sy + ey;
        }
        else if (area->priv->resize_handle == HANDLE_BC)
        {
            area->priv->endy = ey;
            area->priv->g_endy = sy + ey;
        }
        else if (area->priv->resize_handle == HANDLE_BR)
        {
            area->priv->endx = ex;
            area->priv->endy = ey;
            area->priv->g_endx = sx + ex;
            area->priv->g_endy = sy + ey;

        }
        else if (area->priv->resize_handle == HANDLE_MOVE)
        {
            if (area->priv->move_offsetx == area->priv->move_offsety && area->priv->move_offsety == 0)
            {
                area->priv->move_offsetx = ex - area->priv->startx;
                area->priv->move_offsety = ey - area->priv->starty;
            }
            area->priv->startx = MAX(0, ex - area->priv->move_offsetx);
            area->priv->starty = MAX(0, ey - area->priv->move_offsety);
            area->priv->endx = area->priv->startx + area->priv->width;
            area->priv->endy = area->priv->starty + area->priv->height;

            sw = rect.width;
            sh = rect.height;

            if  (area->priv->endx > sw)
            {
                area->priv->startx -= area->priv->endx - sw;
                area->priv->endx = sw;
            }
            if (area->priv->endy > sh)
            {
                area->priv->starty -= area->priv->endy - sh;
                area->priv->endy = sh;
            }
            area->priv->g_startx = sx + area->priv->startx;
            area->priv->g_starty = sy + area->priv->starty;
            area->priv->g_endx = sx + area->priv->endx;
            area->priv->g_endy = sy + area->priv->endy;
        }
        else
        {
            area->priv->endx = ex;
            area->priv->endy = ey;
            area->priv->g_endx = sx + ex;
            area->priv->g_endy = sy + ey;
        }
        area->priv->width  = area->priv->endx - area->priv->startx;
        area->priv->height = area->priv->endy - area->priv->starty;

    }
    gtk_widget_queue_draw (widget);
    return TRUE;
}

static gboolean
cb_draw_button_press_event (GtkWidget       *widget,
                            GdkEventButton  *event,
                            gpointer         user_data)
{
    double        x, y;
    int           ex, ey;
    double        offsetx, offsety;
    int           gx, gy;
    GdkRectangle  rect;
    int i = 0;

    ScreenArea *area = SCREEN_AREA (user_data);

    rect = get_rectangle_data (area->priv->device);
    ex = event->x;
    ey = event->y;
    gx = rect.x + ex;
    gy = rect.y + ey;

    for(i = 0; i < 9; i++)
    {
        x = floor(i % 3) / 2;
        y = floor(i / 3) / 2;
        offsetx = area->priv->width * x;
        offsety = area->priv->height * y;

        if (in_circle(area->priv->g_startx + offsetx,
                      area->priv->g_starty + offsety,
                      8, gx, gy))
        {
            area->priv->resize_handle = i;
            return TRUE;
        }
    }
    if (MIN(area->priv->startx, area->priv->endx) < ex &&
        ex < MAX(area->priv->startx, area->priv->endx) &&\
        MIN(area->priv->starty, area->priv->endy) < ey &&
        ey < MAX(area->priv->starty, area->priv->endy))
    {
        if (event->type == GDK_2BUTTON_PRESS)
        {
            accept_area(area);
            g_signal_emit (area, signals[SELECTED], 0);
        }

        area->priv->resize_handle = HANDLE_MOVE;
        return TRUE;
    }
    area->priv->startx = ex;
    area->priv->starty = ey;
    area->priv->g_startx = gx;
    area->priv->g_starty = gy;
    area->priv->endx = 0;
    area->priv->endy = 0;
    area->priv->g_endx = 0;
    area->priv->g_endy = 0;
    area->priv->width  = 0;
    area->priv->height = 0;

    return TRUE;
}

static gboolean
cb_draw_button_release_event (GtkWidget       *widget,
                              GdkEventButton  *event,
                              gpointer         user_data)
{
    ScreenArea *area = SCREEN_AREA (user_data);

    area->priv->resize_handle = -1;
    area->priv->move_offsetx = 0;
    area->priv->move_offsety = 0;

    return TRUE;
}
static gboolean
cb_leave_notify_event (GtkWidget       *widget,
                       GdkEvent        *event,
                       gpointer         user_data)
{
    int           x, y;
    GdkScreen    *screen;
    GdkRectangle  rect;

    ScreenArea *area = SCREEN_AREA (user_data);
    gdk_device_get_position (area->priv->device, &screen, &x, &y);
    rect = get_rectangle_data (area->priv->device);
    if( x > 0 || y > 0)
    {
        gtk_window_unfullscreen (GTK_WINDOW (user_data));
        gtk_window_move (GTK_WINDOW (user_data), rect.x, rect.y);
        gtk_window_fullscreen (GTK_WINDOW (user_data));
    }

    return TRUE;
}
static void screen_area_init_signal (ScreenArea *area, GtkWidget *drawing)
{

    g_signal_connect (GTK_WIDGET (area),
                     "key-press-event",
                      G_CALLBACK (cb_keypress_event),
                      area);

    g_signal_connect (drawing,
                     "draw",
                      G_CALLBACK (cb_draw),
                      area);

    g_signal_connect (drawing,
                     "motion-notify-event",
                      G_CALLBACK (cb_draw_motion_notify_event),
                      area);

    g_signal_connect (drawing,
                     "button-press-event",
                      G_CALLBACK (cb_draw_button_press_event),
                      area);

    g_signal_connect (drawing,
                     "button-release-event",
                      G_CALLBACK (cb_draw_button_release_event),
                      area);

    g_signal_connect (drawing,
                     "leave-notify-event",
                      G_CALLBACK (cb_leave_notify_event),
                      area);
}

static void
screen_area_fill (ScreenArea *area)
{
    GtkWindow  *window;
    GtkWidget  *vbox;
    GtkWidget  *drawing;

    window = GTK_WINDOW (area);

    vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add (GTK_CONTAINER (window), vbox);
    drawing = gtk_drawing_area_new ();
    gtk_box_pack_start (GTK_BOX (vbox), drawing, TRUE, TRUE, 0);
    gtk_widget_set_size_request (drawing, 100, 100);
    gtk_widget_add_events(drawing, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
                          GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK |
                          GDK_LEAVE_NOTIFY_MASK);
    screen_area_init_signal (area, drawing);
    gtk_window_fullscreen (GTK_WINDOW (window));
}
static GObject *
screen_area_constructor (GType                  type,
                         guint                  n_construct_properties,
                         GObjectConstructParam *construct_properties)
{
    GObject      *obj;
    ScreenArea   *area;

    obj = G_OBJECT_CLASS (screen_area_parent_class)->constructor (type,
                          n_construct_properties,
                          construct_properties);

    area = SCREEN_AREA (obj);
    screen_area_fill (area);

    return obj;
}

static void
screen_area_dispose (GObject *object)
{
    //ScreenArea *area;

    //area = SCREEN_AREA (object);

    G_OBJECT_CLASS (screen_area_parent_class)->dispose (object);
}

static void
screen_area_class_init (ScreenAreaClass *klass)
{
    GObjectClass   *gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->constructor = screen_area_constructor;
    gobject_class->dispose = screen_area_dispose;

    signals [SELECTED] =
         g_signal_new ("selected",
                       G_TYPE_FROM_CLASS (klass),
                       G_SIGNAL_RUN_LAST,
                       0,
                       NULL, NULL,
                       g_cclosure_marshal_VOID__VOID,
                       G_TYPE_NONE, 0);

    signals [CANCELED] =
         g_signal_new ("canceled",
                       G_TYPE_FROM_CLASS (klass),
                       G_SIGNAL_RUN_LAST,
                       0,
                       NULL, NULL,
                       g_cclosure_marshal_VOID__VOID,
                       G_TYPE_NONE, 0);
}
static void
screen_area_init (ScreenArea *area)
{
    GtkWindow  *window;

   // GdkCursor  *cursor;
    GdkScreen  *screen;
    GdkDisplay *display;
    GdkVisual  *visual;
    GdkWindow  *root_window;
    GdkSeat    *seat;

    area->priv = screen_area_get_instance_private (area);
    window = GTK_WINDOW (area);

    screen = gtk_widget_get_screen (GTK_WIDGET (window));
    visual = gdk_screen_get_rgba_visual (screen);
    root_window = gdk_screen_get_root_window (screen);
    display = gdk_window_get_display (root_window);
    area->priv->last_cursor = gdk_cursor_new_for_display (display, GDK_LEFT_PTR);

    seat = gdk_display_get_default_seat (display);
    //cursor = gdk_cursor_new_for_display (display, GDK_CROSSHAIR);
    //gdk_window_set_cursor(root_window, cursor);

    gtk_container_set_border_width (GTK_CONTAINER (window), 0);
    gtk_widget_set_app_paintable (GTK_WIDGET (window), TRUE);
    gtk_window_set_resizable (GTK_WINDOW (window), TRUE);
    gtk_window_set_decorated (GTK_WINDOW (window), FALSE);
    g_object_set (window, "skip-taskbar-hint", TRUE, NULL);
    gtk_window_set_keep_above (GTK_WINDOW (window), TRUE);

    area->priv->compositing = FALSE;
    area->priv->resize_handle = -1;
    if (visual != NULL && gdk_screen_is_composited (screen))
    {
        gtk_widget_set_visual (GTK_WIDGET (window), visual);
        area->priv->compositing = TRUE;

    }
    area->priv->device = gdk_seat_get_pointer (seat);
    area->priv->root_window = root_window;
}

GtkWidget *
screen_area_new (void)
{
    ScreenArea *area;

    area = g_object_new (SCREEN_TYPE_AREA,
                        "type", GTK_WINDOW_TOPLEVEL,
                         NULL);

    return GTK_WIDGET (area);
}

gint32
screen_area_get_height (ScreenArea *area)
{
    int scale;
    g_return_val_if_fail (SCREEN_IS_AREA (area), -1);

    scale = get_window_scaling ();
    return area->priv->height * scale;
}

gint32
screen_area_get_width (ScreenArea *area)
{
    int scale;
    g_return_val_if_fail (SCREEN_IS_AREA (area), -1);

    scale = get_window_scaling ();
    return area->priv->width * scale;
}

gint32
screen_area_get_starty (ScreenArea *area)
{
    int scale;
    g_return_val_if_fail (SCREEN_IS_AREA (area), -1);

    scale = get_window_scaling ();
    return area->priv->starty * scale;
}

gint32
screen_area_get_startx (ScreenArea *area)
{
    int scale;
    g_return_val_if_fail (SCREEN_IS_AREA (area), -1);

    scale = get_window_scaling ();
    return area->priv->startx * scale;
}
