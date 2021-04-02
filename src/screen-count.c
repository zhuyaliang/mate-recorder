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
 */

#include "screen-count.h"

enum
{
    FINISHED,
    LAST_SIGNAL
};

struct _ScreenCountPrivate
{
    int         count_down;
    guint       timeout_id;
    GtkWidget  *spin_button;
    GtkWidget  *window;
};

static guint signals[LAST_SIGNAL] = { 0 };
G_DEFINE_TYPE_WITH_PRIVATE (ScreenCount, screen_count, GTK_TYPE_FRAME)

const char *count_images[] = {"cb-1.png","cb-2.png","cb-3.png","cb-4.png","cb-5.png","cb-6.png","cb-7.png","cb-8.png","cb-9.png","cb-10.png"};

static gboolean on_darw (GtkWidget *widget, cairo_t *cr1, gpointer data)
{
    ScreenCount *count = SCREEN_COUNT (data);
    cairo_surface_t *image;
    GdkWindow       *window;
    cairo_region_t  *region;
    GdkDrawingContext *ctx;
    cairo_t         *cr;
    char            *file;
    int              i;

    i = count->priv->count_down;
    file = g_strdup_printf ("%s/%s","/usr/share/mate-recorder/counter", count_images[i]);
    window = gtk_widget_get_window (widget);
    image = cairo_image_surface_create_from_png (file);

    region = gdk_cairo_region_create_from_surface (image);
    ctx = gdk_window_begin_draw_frame (window, region);
    cr = gdk_drawing_context_get_cairo_context (ctx);

    cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 0.45);
    cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
    cairo_paint (cr);
    cairo_set_source_surface (cr, image, 0, 0);
    cairo_paint (cr);

    gdk_window_end_draw_frame (window, ctx);
    cairo_region_destroy (region);
    cairo_surface_destroy (image);

    g_free (file);
    return FALSE;
}

static gboolean send_finished_signal (gpointer data)
{
    ScreenCount *count = SCREEN_COUNT (data);
    g_signal_emit (count, signals[FINISHED], 0);

    return FALSE;
}

static gboolean screen_countdown (gpointer data)
{
    ScreenCount *count = SCREEN_COUNT (data);
    gint value;

    gtk_widget_queue_resize (count->priv->window);
    if (count->priv->count_down <= 0)
    {
        g_source_remove (count->priv->timeout_id);
        count->priv->timeout_id = 0;
        gtk_widget_hide (count->priv->window);
        g_timeout_add (1000, (GSourceFunc)send_finished_signal, count);
        value = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (count->priv->spin_button));
        count->priv->count_down = value;

        return FALSE;
    }
    //gtk_widget_show (count->priv->window );
    count->priv->count_down -= 1;

    return TRUE;
}

static void count_down_changed_cb (GtkSpinButton *spin_button,
                                   gpointer       user_data)
{
    ScreenCount *count = SCREEN_COUNT (user_data);
    gint value;

    value = gtk_spin_button_get_value_as_int (spin_button);
    count->priv->count_down = value;
}

static GtkWidget *create_count_down_window (ScreenCount *count)
{
    GtkWidget *window;
    GtkWidget *darea;
    GdkVisual *visual;
    GtkWidget *toplevel;
    GdkScreen *screen;

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    darea = gtk_drawing_area_new ();
    gtk_container_add (GTK_CONTAINER (window), darea);

    g_signal_connect (darea, "draw", G_CALLBACK (on_darw), count);
    gtk_widget_show (darea);
    toplevel = gtk_widget_get_toplevel (window);
    screen = gtk_widget_get_screen (GTK_WIDGET (toplevel));
    visual = gdk_screen_get_rgba_visual (screen);
    gtk_widget_set_visual (GTK_WIDGET (toplevel), visual);

    gtk_window_set_position (GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    gtk_window_set_type_hint (GTK_WINDOW(window), GDK_WINDOW_TYPE_HINT_MENU);
    gtk_window_set_default_size (GTK_WINDOW(window), 380, 380);
    gtk_widget_set_app_paintable (window, TRUE);
    gtk_window_set_resizable (GTK_WINDOW(window), TRUE);
    gtk_window_set_decorated (GTK_WINDOW(window), FALSE);

    return window;
}

static void
screen_count_dispose (GObject *object)
{
    ScreenCount *count = SCREEN_COUNT (object);
    if (count->priv->timeout_id != 0)
    {
        g_source_remove (count->priv->timeout_id);
        count->priv->timeout_id = 0;
    }
}

static void
screen_count_init (ScreenCount *count)
{
    GtkWidget *hbox;
    GtkWidget *label;
    GtkWidget *spin;

    count->priv = screen_count_get_instance_private (count);
    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_widget_set_valign (hbox, GTK_ALIGN_CENTER);
    gtk_widget_set_halign (hbox, GTK_ALIGN_CENTER);
    gtk_container_add (GTK_CONTAINER (count), hbox);
    label = gtk_label_new (_("count down"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 12);
    spin = gtk_spin_button_new_with_range (0, 10, 1);
    gtk_widget_set_valign (spin, GTK_ALIGN_CENTER);
    gtk_widget_set_halign (spin, GTK_ALIGN_CENTER);
    g_signal_connect (spin,
                     "value-changed",
                      G_CALLBACK (count_down_changed_cb),
                      count);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin), 5);
    gtk_box_pack_start (GTK_BOX (hbox), spin, FALSE, FALSE, 12);

    count->priv->spin_button = spin;
    count->priv->window = create_count_down_window (count);
}

static void
screen_count_class_init (ScreenCountClass *count_class)
{
    GObjectClass *gobject_class;

    gobject_class = G_OBJECT_CLASS (count_class);
    gobject_class->dispose = screen_count_dispose;

    signals [FINISHED] =
         g_signal_new ("finished",
                       G_TYPE_FROM_CLASS (count_class),
                       G_SIGNAL_RUN_LAST,
                       0,
                       NULL, NULL,
                       g_cclosure_marshal_VOID__VOID,
                       G_TYPE_NONE, 0);

}

GtkWidget *
screen_count_new (const char *title)
{
    ScreenCount *count = NULL;
    GtkWidget   *label;
    char        *text;

    count = g_object_new (SCREEN_TYPE_COUNT, NULL);
    gtk_frame_set_label (GTK_FRAME (count), "");
    text =  g_markup_printf_escaped ("<span color = \'grey\' size=\"%s\" weight='bold'>%s</span>","large",title);
    label = gtk_frame_get_label_widget (GTK_FRAME (count));
    gtk_label_set_markup (GTK_LABEL (label), text);

    g_free (text);

    return GTK_WIDGET (count);
}

gboolean screen_start_count_down (ScreenCount *count)
{
    g_return_val_if_fail (SCREEN_IS_COUNT (count), FALSE);
    g_return_val_if_fail (count->priv->window != NULL, FALSE);

    if (count->priv->count_down == 0)
    {
        g_signal_emit (count, signals[FINISHED], 0);
        return TRUE;
    }
    count->priv->count_down --;
    gtk_widget_queue_resize (count->priv->window);
    count->priv->timeout_id = g_timeout_add (1000, (GSourceFunc)screen_countdown, count);
    gtk_widget_show (count->priv->window);

    return TRUE;
}

void screen_stop_count_down (ScreenCount *count)
{
    gint value;
    
    if (count->priv->timeout_id != 0)
    {
        g_source_remove (count->priv->timeout_id);
        count->priv->timeout_id = 0;
    }
    gtk_widget_hide (count->priv->window);
    value = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (count->priv->spin_button));
    count->priv->count_down = value;
    g_timeout_add (500, (GSourceFunc)send_finished_signal, count);
}
