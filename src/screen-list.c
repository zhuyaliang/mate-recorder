/*************************************************************************
  File Name: screen-list.c

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

  Created Time: 2021年02月04日 星期四 17时40分57秒
 ************************************************************************/
#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <unistd.h>
#include <libwnck/libwnck.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include "screen-list.h"

enum
{
    SELECTED,
    CANCELED,
    LAST_SIGNAL
};
struct _ScreenListPrivate
{
    WnckWindow   *wnck_window;

    GtkListStore *store;
    GtkWidget    *tree_view;
    GtkWidget    *button;
};

static guint signals[LAST_SIGNAL] = { 0 };
G_DEFINE_TYPE_WITH_PRIVATE (ScreenList, screen_list, GTK_TYPE_DIALOG)

static GtkWidget* dialog_add_button_with_icon_name (GtkDialog   *dialog,
                                                    const gchar *button_text,
                                                    const gchar *icon_name,
                                                    gint         response_id)
{
    GtkWidget *button;

    button = gtk_button_new_with_mnemonic (button_text);
    gtk_button_set_image (GTK_BUTTON (button), gtk_image_new_from_icon_name (icon_name, GTK_ICON_SIZE_BUTTON));

    gtk_button_set_use_underline (GTK_BUTTON (button), TRUE);
    gtk_style_context_add_class (gtk_widget_get_style_context (button), "text-button");
    gtk_widget_set_can_default (button, TRUE);
    gtk_widget_show (button);
    gtk_dialog_add_action_widget (GTK_DIALOG (dialog), button, response_id);

    return button;
}
static void
set_window_icons_cb (GtkTreeViewColumn *column,
                     GtkCellRenderer   *renderer,
                     GtkTreeModel      *model,
                     GtkTreeIter       *iter,
                     gpointer           data)
{
    WnckWindow *window;
    GdkPixbuf  *pixbuf;

    gtk_tree_model_get (model,
                        iter,
                        0,
                        &window,
                        -1);
    if (window == NULL)
    {
        return;
    }
    
    pixbuf = wnck_window_get_mini_icon (window);
    g_object_set (renderer, "pixbuf", pixbuf, NULL);
    g_object_unref (G_OBJECT (window));
}

static void
set_window_title_cb (GtkTreeViewColumn *column,
                     GtkCellRenderer   *renderer,
                     GtkTreeModel      *model,
                     GtkTreeIter       *iter,
                     gpointer           data)
{
    WnckWindow *window;
    const char *mini_name;

    gtk_tree_model_get (model,
                        iter,
                        0,
                        &window,
                        -1);
    if (window == NULL)
    {
        return;
    }
    mini_name = wnck_window_get_name (window);
    g_object_set (renderer, "text", mini_name, NULL);
    g_object_unref (G_OBJECT (window));
}

static void
create_tree_view_column (GtkWidget *tree_view)
{
    GtkTreeViewColumn *column;
    GtkCellRenderer   *renderer;

    column = gtk_tree_view_column_new ();
    gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (column),
                                     GTK_TREE_VIEW_COLUMN_FIXED);

    renderer = gtk_cell_renderer_pixbuf_new ();
    g_object_set (G_OBJECT (renderer), "stock-size", 8, NULL);
    gtk_cell_renderer_set_padding (renderer, 8, 8);

    gtk_tree_view_column_pack_start (column,
                                     renderer,
                                     FALSE);

    gtk_tree_view_column_set_cell_data_func (column, 
                                             renderer,
                                             set_window_icons_cb,
                                             NULL,
                                             NULL);

    renderer = gtk_cell_renderer_text_new ();
    gtk_tree_view_column_pack_start (column,
                                     renderer,
                                     TRUE);

    gtk_tree_view_column_set_cell_data_func (column,
                                             renderer,
                                             set_window_title_cb,
                                             NULL,
                                             NULL);

    gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);
    gtk_tree_view_set_headers_visible (GTK_TREE_VIEW(tree_view), FALSE);
}

static gboolean  
select_window_list_cb (GtkWidget *widget, gpointer data)
{
    GtkTreeIter   iter;
    GtkTreeModel *model;
    WnckWindow   *window;
    
    ScreenList *list = SCREEN_LIST (data);
    if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(widget), &model, &iter))
    {

        gtk_tree_model_get (model,
                            &iter,
                            0,
                            &window,
                            -1);
    }
    if (window == NULL)
    { 
            return FALSE;
    }
    list->priv->wnck_window = window;
    gtk_widget_set_sensitive (list->priv->button, TRUE);
    
    return TRUE;
}

static void
screen_list_dialog_fill (ScreenList *list)
{
    GtkWidget    *dialog_area;
    GtkWidget    *sw;
    GtkDialog    *dialog;
    GtkTreeSelection *selection;

    dialog = GTK_DIALOG (list);
    dialog_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
    list->priv->store = gtk_list_store_new (1, WNCK_TYPE_WINDOW);
    list->priv->tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(list->priv->store));
    create_tree_view_column (list->priv->tree_view);

    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (list->priv->tree_view));
    gtk_tree_selection_set_mode (selection,GTK_SELECTION_SINGLE);
    g_signal_connect(selection,
                    "changed",
                     G_CALLBACK(select_window_list_cb),
                     list);

    sw = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
                                    GTK_POLICY_AUTOMATIC,
                                    GTK_POLICY_AUTOMATIC);
    gtk_container_add (GTK_CONTAINER (sw), list->priv->tree_view);
    gtk_box_pack_start (GTK_BOX (dialog_area), sw, TRUE, TRUE, 6);


}

static GObject *
screen_list_constructor (GType                  type,
                         guint                  n_construct_properties,
                         GObjectConstructParam *construct_properties)
{
    GObject      *obj;
    ScreenList   *list;

    obj = G_OBJECT_CLASS (screen_list_parent_class)->constructor (type,
                          n_construct_properties,
                          construct_properties);

    list = SCREEN_LIST (obj);
    screen_list_dialog_fill (list);

    return obj;
}

static void
refresh_screen_window_list (ScreenList *list)
{
    GList      *l;
    WnckScreen *screen;
    WnckWindow *window;
    GtkTreeIter iter;
    int         pid,own_pid;

    own_pid = getpid ();
    gtk_list_store_clear (list->priv->store);

    screen = wnck_screen_get (0);
    l = wnck_screen_get_windows (screen);
    while (l != NULL)
    {
        window = l->data;
        pid = wnck_window_get_pid (window);

        if (wnck_window_is_pinned (window) || pid == own_pid)
        {
            l = l->next;
            continue;
        }
        gtk_list_store_append (list->priv->store, &iter);
        gtk_list_store_set (list->priv->store, &iter, 0, window, -1);

        l = l->next;
    }
}

static void
window_opened_callback (WnckScreen    *screen,
                        WnckWindow    *window,
                        gpointer       data)
{
    ScreenList *list = SCREEN_LIST (data);

    refresh_screen_window_list (list);
}

static void
window_closed_callback (WnckScreen    *screen,
                        WnckWindow    *window,
                        gpointer       data)
{
    ScreenList *list = SCREEN_LIST (data);

    refresh_screen_window_list (list);
}

static void
screen_list_dispose (GObject *object)
{
    //ScreenList *list;

    //list = SCREEN_LIST (object);

    G_OBJECT_CLASS (screen_list_parent_class)->dispose (object);
}

static void
screen_list_dialog_response  (GtkDialog *dialog,
                              gint response_id)
{
    ScreenList *list = SCREEN_LIST (dialog);
    switch (response_id)
    {
        case GTK_RESPONSE_OK:
            g_signal_emit (list, signals[SELECTED], 0);
            gtk_widget_hide (GTK_WIDGET (dialog));
            break;
        case GTK_RESPONSE_CANCEL:
            g_signal_emit (list, signals[CANCELED], 0);
            gtk_widget_hide (GTK_WIDGET (dialog));
            break;
        default:
            break;
    }
}

static void
screen_list_class_init (ScreenListClass *klass)
{
    GObjectClass   *gobject_class = G_OBJECT_CLASS (klass);
    GtkDialogClass *dialog_class = GTK_DIALOG_CLASS (klass);

    gobject_class->constructor = screen_list_constructor;
    gobject_class->dispose = screen_list_dispose;
    dialog_class->response = screen_list_dialog_response;

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
screen_list_init (ScreenList *list)
{
    GtkDialog  *dialog;
    GtkWidget  *button;
    WnckScreen *screen;

    list->priv = screen_list_get_instance_private (list);
    list->priv->wnck_window = NULL;

    dialog = GTK_DIALOG (list);

    dialog_add_button_with_icon_name (dialog,
                                      _("Return"),
                                      "window-close",
                                       GTK_RESPONSE_CANCEL);

    button = dialog_add_button_with_icon_name (dialog,
                                             _("Confirm"),
                                               "emblem-default",
                                                GTK_RESPONSE_OK);

    gtk_widget_set_sensitive (button, FALSE);
    list->priv->button = button;
    gtk_window_set_default_size (GTK_WINDOW (dialog), 300, 300);
    gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);

    screen = wnck_screen_get (0);
    g_signal_connect (G_OBJECT (screen),
                     "window_opened",
                      G_CALLBACK (window_opened_callback),
                      list);

    g_signal_connect (G_OBJECT (screen),
                     "window_closed",
                      G_CALLBACK (window_closed_callback),
                      list);

}

GtkWidget *
screen_list_new (void)
{
    ScreenList *list;

    list = g_object_new (SCREEN_TYPE_LIST,
                        "type", GTK_WINDOW_POPUP,
                         NULL);

    return GTK_WIDGET (list);
}

gulong screen_list_get_window_xid (ScreenList *list)
{
    gulong xid;
    if (list->priv->wnck_window == NULL)
    {
        return 0;
    }

    xid = wnck_window_get_xid (list->priv->wnck_window);

    return xid; 
}

void screnn_set_window_activate (ScreenList *list)
{

    if (list->priv->wnck_window == NULL)
        return;

    if (wnck_window_is_active (list->priv->wnck_window))
        return;

    wnck_window_activate (list->priv->wnck_window, gtk_get_current_event_time ());
} 
