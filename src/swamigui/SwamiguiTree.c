/*
 * SwamiguiTree.c - Swami tabbed tree object
 *
 * Swami
 * Copyright (C) 1999-2014 Element Green <element@elementsofsound.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 2
 * of the License only.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA or point your web browser to http://www.gnu.org.
 */
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <gtk/gtk.h>
#include <pango/pango.h>

#include <libinstpatch/libinstpatch.h>
#include <libswami/libswami.h>

#include "SwamiguiTree.h"
#include "SwamiguiTreeStore.h"
#include "SwamiguiItemMenu.h"
#include "SwamiguiRoot.h"
#include "SwamiguiDnd.h"
#include "i18n.h"


/* properties */
enum
{
    PROP_0,
    PROP_SELECTION_SINGLE,
    PROP_SELECTION,
    PROP_SELECTED_STORE,	/* currently selected store */
    PROP_STORE_LIST	/* list of tree store objects (multi-tabbed tree) */
};

/* Local Prototypes */

static void swamigui_tree_class_init(SwamiguiTreeClass *klass);
static void swamigui_tree_set_property(GObject *object, guint property_id,
                                       const GValue *value, GParamSpec *pspec);
static void swamigui_tree_get_property(GObject *object, guint property_id,
                                       GValue *value, GParamSpec *pspec);
static void swamigui_tree_init(SwamiguiTree *tree);
static void swamigui_tree_cb_switch_page(GtkNotebook *notebook,
        GtkNotebookPage *page,
        guint page_num, gpointer user_data);
static void
swamigui_tree_cb_drag_data_received(GtkWidget *widget, GdkDragContext *context,
                                    gint x, gint y,
                                    GtkSelectionData *selection_data,
                                    guint info, guint time, gpointer data);
static void swamigui_tree_cb_drag_data_get(GtkWidget *widget,
        GdkDragContext *drag_context,
        GtkSelectionData *data, guint info,
        guint time, gpointer user_data);
static void swamigui_tree_real_set_store(SwamiguiTree *tree,
        SwamiguiTreeStore *store);
static void swamigui_tree_update_selection(SwamiguiTree *tree);
static void tree_selection_foreach_func(GtkTreeModel *model,
                                        GtkTreePath *path, GtkTreeIter *iter,
                                        gpointer data);
static void swamigui_tree_finalize(GObject *object);
static gboolean swamigui_tree_widget_popup_menu(GtkWidget *widg);
static void swamigui_tree_cb_search_entry_changed(GtkEntry *entry,
        gpointer user_data);
static void
swamigui_tree_cb_search_next_clicked(GtkButton *button, gpointer user_data);
static void
swamigui_tree_cb_search_prev_clicked(GtkButton *button, gpointer user_data);
static GtkWidget *
swamigui_tree_create_scrolled_tree_view(SwamiguiTree *tree,
                                        SwamiguiTreeStore *store,
                                        GtkTreeView **out_treeview);
static void swamigui_tree_item_icon_cell_data(GtkTreeViewColumn *tree_column,
        GtkCellRenderer *cell,
        GtkTreeModel *tree_model,
        GtkTreeIter *iter,
        gpointer data);
static void swamigui_tree_item_label_cell_data(GtkTreeViewColumn *tree_column,
        GtkCellRenderer *cell,
        GtkTreeModel *tree_model,
        GtkTreeIter *iter,
        gpointer data);
static void tree_cb_selection_changed(GtkTreeSelection *selection,
                                      SwamiguiTree *tree);
static gboolean swamigui_tree_cb_button_press(GtkWidget *widg,
        GdkEventButton *event,
        SwamiguiTree *tree);
static void swamigui_tree_do_popup_menu(SwamiguiTree *tree,
                                        GObject *rclick_item,
                                        GdkEventButton *event);
static void swamigui_tree_set_selection_real(SwamiguiTree *tree,
        IpatchList *list,
        int notify_flags);
static void swamigui_tree_real_search_next(SwamiguiTree *tree, gboolean usematch);
static void set_search_match_item(SwamiguiTree *tree, GtkTreeIter *iter,
                                  GObject *obj, int startpos, const char *search);
static void reset_search_match_item(SwamiguiTree *tree, GList **new_ancestry);
static int str_index(const char *haystack, const char *needle);
static gboolean tree_iter_recursive_next(GtkTreeModel *model, GtkTreeIter *iter);
static gboolean tree_iter_recursive_prev(GtkTreeModel *model, GtkTreeIter *iter);

static GObjectClass *parent_class = NULL;

GType
swamigui_tree_get_type(void)
{
    static GType obj_type = 0;

    if(!obj_type)
    {
        static const GTypeInfo obj_info =
        {
            sizeof(SwamiguiTreeClass), NULL, NULL,
            (GClassInitFunc) swamigui_tree_class_init, NULL, NULL,
            sizeof(SwamiguiTree), 0,
            (GInstanceInitFunc) swamigui_tree_init,
        };

        obj_type = g_type_register_static(GTK_TYPE_VBOX, "SwamiguiTree",
                                          &obj_info, 0);
    }

    return (obj_type);
}

static void
swamigui_tree_class_init(SwamiguiTreeClass *klass)
{
    GObjectClass *obj_class = G_OBJECT_CLASS(klass);
    GtkWidgetClass *widg_class = GTK_WIDGET_CLASS(klass);

    parent_class = g_type_class_peek_parent(klass);

    widg_class->popup_menu = swamigui_tree_widget_popup_menu;

    obj_class->set_property = swamigui_tree_set_property;
    obj_class->get_property = swamigui_tree_get_property;
    obj_class->finalize = swamigui_tree_finalize;

    g_object_class_install_property(obj_class, PROP_SELECTION_SINGLE,
                                    g_param_spec_object("selection-single", "Single selection",
                                            "Single selected object",
                                            G_TYPE_OBJECT, /* FIXME? */
                                            G_PARAM_READWRITE));
    g_object_class_install_property(obj_class, PROP_SELECTION,
                                    g_param_spec_object("selection", "Selection",
                                            "Selection list (static)",
                                            IPATCH_TYPE_LIST,
                                            G_PARAM_READWRITE));
    g_object_class_install_property(obj_class, PROP_SELECTED_STORE,
                                    g_param_spec_object("selected-store", "Selection store",
                                            "Selected tree store",
                                            SWAMIGUI_TYPE_TREE_STORE,
                                            G_PARAM_READWRITE));
    g_object_class_install_property(obj_class, PROP_STORE_LIST,
                                    g_param_spec_object("store-list", "Store list",
                                            "Tree store list",
                                            IPATCH_TYPE_LIST,
                                            G_PARAM_READWRITE));
}

static void
swamigui_tree_set_property(GObject *object, guint property_id,
                           const GValue *value, GParamSpec *pspec)
{
    SwamiguiTree *tree = SWAMIGUI_TREE(object);
    SwamiguiTreeStore *store;
    IpatchList *list = NULL;
    GObject *item;

    switch(property_id)
    {
    case PROP_SELECTION_SINGLE:
        item = g_value_get_object(value);

        if(item)
        {
            list = ipatch_list_new();  /* ++ ref new list */
            list->items = g_list_append(list->items, g_object_ref(item));
        }

        swamigui_tree_set_selection_real(tree, list, 1);

        if(list)
        {
            g_object_unref(list);    /* -- unref list */
        }

        break;

    case PROP_SELECTION:
        list = g_value_get_object(value);
        swamigui_tree_set_selection_real(tree, list, 2);
        break;

    case PROP_SELECTED_STORE:
        store = SWAMIGUI_TREE_STORE(g_value_get_object(value));
        swamigui_tree_set_selected_store(tree, store);
        break;

    case PROP_STORE_LIST:
        list = IPATCH_LIST(g_value_get_object(value));
        swamigui_tree_set_store_list(tree, list);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

static void
swamigui_tree_get_property(GObject *object, guint property_id,
                           GValue *value, GParamSpec *pspec)
{
    SwamiguiTree *tree = SWAMIGUI_TREE(object);
    GObject *item;
    IpatchList *list;

    switch(property_id)
    {
    case PROP_SELECTION_SINGLE:
        item = swamigui_tree_get_selection_single(tree);
        g_value_set_object(value, item);
        break;

    case PROP_SELECTION: /* get tree selection (uses directly without ref!) */
        list = swamigui_tree_get_selection(tree);
        g_value_set_object(value, list);
        break;

    case PROP_SELECTED_STORE:
        g_value_set_object(value, tree->selstore);
        break;

    case PROP_STORE_LIST:
        g_value_set_object(value, tree->stores);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

static void
swamigui_tree_init(SwamiguiTree *tree)
{
    GtkWidget *widg;
    GtkWidget *image;

    tree->notebook = GTK_NOTEBOOK(gtk_notebook_new());
    gtk_widget_show(GTK_WIDGET(tree->notebook));
    gtk_box_pack_start(GTK_BOX(tree), GTK_WIDGET(tree->notebook),
                       TRUE, TRUE, 0);

    /* add the search widgets */
    tree->search_box = gtk_hbox_new(FALSE, 2);
    gtk_box_pack_start(GTK_BOX(tree), tree->search_box, FALSE, FALSE, 2);

    image = gtk_image_new_from_stock(GTK_STOCK_CLOSE, GTK_ICON_SIZE_MENU);
    widg = gtk_button_new();
    gtk_button_set_image(GTK_BUTTON(widg), image);
    gtk_button_set_relief(GTK_BUTTON(widg), GTK_RELIEF_NONE);
    gtk_box_pack_start(GTK_BOX(tree->search_box), widg, FALSE, FALSE, 0);

    widg = gtk_label_new(_("Search"));
    gtk_box_pack_start(GTK_BOX(tree->search_box), widg, FALSE, FALSE, 2);

    tree->search_entry = GTK_ENTRY(gtk_entry_new());
    g_signal_connect(G_OBJECT(tree->search_entry), "changed",
                     G_CALLBACK(swamigui_tree_cb_search_entry_changed), tree);
    gtk_box_pack_start(GTK_BOX(tree->search_box),
                       GTK_WIDGET(tree->search_entry), TRUE, TRUE, 0);

    image = gtk_image_new_from_stock(GTK_STOCK_GO_BACK, GTK_ICON_SIZE_MENU);
    widg = gtk_button_new();
    gtk_button_set_image(GTK_BUTTON(widg), image);
    gtk_button_set_relief(GTK_BUTTON(widg), GTK_RELIEF_NONE);
    gtk_box_pack_start(GTK_BOX(tree->search_box), widg, FALSE, FALSE, 0);
    g_signal_connect(G_OBJECT(widg), "clicked",
                     G_CALLBACK(swamigui_tree_cb_search_prev_clicked), tree);

    image = gtk_image_new_from_stock(GTK_STOCK_GO_FORWARD, GTK_ICON_SIZE_MENU);
    widg = gtk_button_new();
    gtk_button_set_image(GTK_BUTTON(widg), image);
    gtk_button_set_relief(GTK_BUTTON(widg), GTK_RELIEF_NONE);
    gtk_box_pack_start(GTK_BOX(tree->search_box), widg, FALSE, FALSE, 0);
    g_signal_connect(G_OBJECT(widg), "clicked",
                     G_CALLBACK(swamigui_tree_cb_search_next_clicked), tree);

    gtk_widget_show_all(tree->search_box);

    /* attach to the "switch-page" signal to update selection when the current
       notebook page is changed */
    g_signal_connect(tree->notebook, "switch-page",
                     G_CALLBACK(swamigui_tree_cb_switch_page), tree);
}

/* callback when notebook page changes */
static void
swamigui_tree_cb_switch_page(GtkNotebook *notebook, GtkNotebookPage *page,
                             guint page_num, gpointer user_data)
{
    SwamiguiTree *tree = SWAMIGUI_TREE(user_data);
    SwamiguiTreeStore *store;

    if(!tree->stores)
    {
        return;
    }

    store = g_list_nth_data(tree->stores->items, page_num);
    swamigui_tree_real_set_store(tree, store);
}

static void
swamigui_tree_real_set_store(SwamiguiTree *tree, SwamiguiTreeStore *store)
{
    int n;

    if(!tree->stores)
    {
        return;
    }

    n = g_list_index(tree->stores->items, store);

    if(n == -1)
    {
        return;
    }

    if(store != tree->selstore)
    {
        tree->selstore = store;
        tree->seltree = g_list_nth_data(tree->treeviews, n);

        /* update tree selection to that of the current selected tree view */
        swamigui_tree_update_selection(tree);
    }
}

/* update the tree selection to the currently selected tree */
static void
swamigui_tree_update_selection(SwamiguiTree *tree)
{
    GtkTreeSelection *selection;
    gboolean new_sel_single;
    GList *list = NULL;

    if(!tree->seltree)
    {
        return;    /* shouldn't happen, but just in case */
    }

    selection = gtk_tree_view_get_selection(tree->seltree);

    if(tree->selection)
    {
        g_object_unref(tree->selection);    /* -- unref old sel */
    }

    /* convert tree selection to list */
    gtk_tree_selection_selected_foreach(selection,
                                        tree_selection_foreach_func,
                                        &list);

    tree->selection = ipatch_list_new();  /* ++ ref new list object */

    /* set the origin object of the selection, so swamigui_root can report
       this to those who want to know. */
    swami_object_set_origin(G_OBJECT(tree->selection), G_OBJECT(tree));

    if(list)
    {
        list = g_list_reverse(list);	/* prepended, so reverse it */
        tree->selection->items = list;
    }

    /* notify single selection change if new single selected or was single */
    new_sel_single = list && !list->next;

    if(new_sel_single || tree->sel_single)
    {
        g_object_notify(G_OBJECT(tree), "selection-single");
    }

    tree->sel_single = new_sel_single;
    g_object_notify(G_OBJECT(tree), "selection");   /* notify selection change */
}

static void
tree_selection_foreach_func(GtkTreeModel *model, GtkTreePath *path,
                            GtkTreeIter *iter, gpointer data)
{
    GList **plist = (GList **)data;
    GObject *obj;

    gtk_tree_model_get(model, iter,
                       SWAMIGUI_TREE_STORE_OBJECT_COLUMN, &obj, /* ++ ref obj */
                       -1);

    if(obj)
    {
        *plist = g_list_prepend(*plist, obj);    /* !! list takes over reference */
    }
}


static void
swamigui_tree_finalize(GObject *object)
{
    SwamiguiTree *tree = SWAMIGUI_TREE(object);

    if(tree->stores)
    {
        g_object_unref(tree->stores);
    }

    if(tree->treeviews)
    {
        g_list_free(tree->treeviews);
    }

    if(tree->selection)
    {
        g_object_unref(tree->selection);
    }
}

static gboolean
swamigui_tree_widget_popup_menu(GtkWidget *widg)
{
    SwamiguiTree *tree = SWAMIGUI_TREE(widg);
    GtkTreeModel *model;
    GtkTreeIter iter;
    GObject *rclick_item = NULL;
    GtkTreePath *path;

    if(!tree->seltree)
    {
        return (TRUE);    /* shouldn't happen, but just in case */
    }

    gtk_tree_view_get_cursor(GTK_TREE_VIEW(tree->seltree), &path, NULL);

    if(path)
    {
        model = gtk_tree_view_get_model(GTK_TREE_VIEW(tree->seltree));

        /* convert path to iter */
        if(gtk_tree_model_get_iter(model, &iter, path))
        {
            rclick_item = swamigui_tree_store_node_get_item
                          (SWAMIGUI_TREE_STORE(model), &iter);
        }

        gtk_tree_path_free(path);
    }

    swamigui_tree_do_popup_menu(tree, rclick_item, NULL);

    return (TRUE);
}

/**
 * swamigui_tree_new:
 * @stores: List of tree stores to use or %NULL to set later
 *   (see swamigui_tree_set_store_list()).
 *
 * Create a new Swami tree object
 *
 * Returns: Swami tree object
 */
GtkWidget *
swamigui_tree_new(IpatchList *stores)
{
    GtkWidget *tree;

    tree = GTK_WIDGET(g_object_new(SWAMIGUI_TYPE_TREE, NULL));

    if(stores)
    {
        swamigui_tree_set_store_list(SWAMIGUI_TREE(tree), stores);
    }

    return (tree);
}

/**
 * swamigui_tree_set_store_list:
 * @tree: Swami GUI tree view
 * @list: List of #SwamiguiTreeStore objects, this list is used directly and
 *   should not be modified after calling this function.
 *
 * Set the tree stores of a tree view.  Each tree store gets its own tab
 * in the tabbed @tree.
 */
void
swamigui_tree_set_store_list(SwamiguiTree *tree, IpatchList *list)
{
    GList *curlist = NULL, *newlist;
    GList *p, *p2;
    int pos, index;
    GtkWidget *page;
    GtkWidget *label;
    GtkTreeView *treeview;
    char *name;

    g_return_if_fail(SWAMIGUI_IS_TREE(tree));
    g_return_if_fail(IPATCH_IS_LIST(list));

    if(tree->stores)
    {
        curlist = tree->stores->items;
    }

    newlist = list->items;

    /* check if current and new store lists are equivalent */
    if(curlist)
    {
        for(p = newlist, p2 = curlist; p && p2; p = p->next, p2 = p2->next)
            if(p->data != p2->data)
            {
                break;
            }

        if(!p && !p2)
        {
            return;    /* they have the same stores? - return */
        }
    }

    /* copy current list so we can modify it (part of an IpatchList) */
    if(curlist)
    {
        curlist = g_list_copy(curlist);
    }

    for(p = newlist, pos = 0; p; p = p->next, pos++)	/* loop over new stores */
    {
        /* store already in current list? */
        if(curlist && (index = g_list_index(curlist, p->data)) != -1)
        {
            if(index != pos)	/* does it need to be moved? */
            {
                /* move the item in our copy of curlist */
                curlist = g_list_remove(curlist, p->data);
                curlist = g_list_insert(curlist, p->data, pos);

                /* move the notebook page */
                page = gtk_notebook_get_nth_page(tree->notebook, index);
                gtk_notebook_reorder_child(tree->notebook, page, pos);

                /* update the treeviews list too */
                treeview = g_list_nth_data(tree->treeviews, index);
                tree->treeviews = g_list_remove(tree->treeviews, treeview);
                tree->treeviews = g_list_insert(tree->treeviews, treeview, pos);
            }

            continue;
        }

        /* keep curlist in sync with what we are doing */
        curlist = g_list_insert(curlist, p->data, pos);

        /* create a new scrolled tree view */
        page = swamigui_tree_create_scrolled_tree_view
               (tree, SWAMIGUI_TREE_STORE(p->data), &treeview);

        swami_object_get(p->data, "name", &name, NULL);

        /* create label for tab */
        label = gtk_label_new(name);
        gtk_widget_show(label);
        g_free(name);

        /* insert into the notebook */
        gtk_notebook_insert_page(tree->notebook, page, label, pos);

        /* update treeviews list also */
        tree->treeviews = g_list_insert(tree->treeviews, treeview, pos);
    }

    /* remove any remaining items in curlist (not part of new store list) */
    for(p = g_list_nth(curlist, pos); p; p = p->next, pos++)
    {
        /* remove notebook page */
        gtk_notebook_remove_page(tree->notebook, pos);

        /* remove the GtkTreeView list node */
        tree->treeviews = g_list_remove_link(tree->treeviews,
                                             g_list_nth(tree->treeviews, pos));
    }

    if(tree->stores)
    {
        g_object_unref(tree->stores);    /* -- unref old stores */
    }

    tree->stores = g_object_ref(list);

    g_list_free(curlist);	/* duplicated list not needed anymore */

    /* Select first tree view if none currently selected. */
    if(!tree->seltree && tree->stores && tree->stores->items)
        swamigui_tree_real_set_store(tree, SWAMIGUI_TREE_STORE
                                     (tree->stores->items->data));
}

static GtkWidget *
swamigui_tree_create_scrolled_tree_view(SwamiguiTree *tree,
                                        SwamiguiTreeStore *store,
                                        GtkTreeView **out_treeview)
{
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkTreeSelection *selection;
    GtkWidget *scrollwin;
    GtkTreeView *treeview;

    GtkTargetEntry target_table[] =
    {
        { SWAMIGUI_DND_OBJECT_NAME, 0, SWAMIGUI_DND_OBJECT_INFO },
        { SWAMIGUI_DND_URI_NAME, 0, SWAMIGUI_DND_URI_INFO }
    };

    scrollwin = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollwin),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_show(scrollwin);

    treeview = GTK_TREE_VIEW(gtk_tree_view_new());

    /* all rows have same height, enable fixed height mode for increased speed
       FIXME - Breaks scroll bar, why??? */
    //  g_object_set (treeview, "fixed-height-mode", TRUE, NULL);

    /* disable interactive search (breaks playing of piano from keyboard) */
    g_object_set(treeview, "enable-search", FALSE, NULL);

    gtk_container_add(GTK_CONTAINER(scrollwin), GTK_WIDGET(treeview));
    gtk_widget_show(GTK_WIDGET(treeview));

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
    gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeview), FALSE);

    /* add pixbuf column to tree view widget */
    renderer = gtk_cell_renderer_pixbuf_new();
    column = gtk_tree_view_column_new();

    // g_object_set (column, "sizing", GTK_TREE_VIEW_COLUMN_FIXED, NULL);

    gtk_tree_view_column_pack_start(column, renderer, FALSE);

    gtk_tree_view_column_set_cell_data_func(column, renderer,
                                            swamigui_tree_item_icon_cell_data,
                                            treeview, NULL);

    /* add label column to tree view widget */
    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(column, renderer, TRUE);

    gtk_tree_view_column_set_cell_data_func(column, renderer,
                                            swamigui_tree_item_label_cell_data,
                                            tree, NULL);

    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

    /* assign the tree store */
    gtk_tree_view_set_model(GTK_TREE_VIEW(treeview), GTK_TREE_MODEL(store));

    /* attach selection changed signal handler */
    g_signal_connect(selection, "changed",
                     G_CALLBACK(tree_cb_selection_changed), tree);

    /* for right click menus */
    g_signal_connect(treeview, "button-press-event",
                     G_CALLBACK(swamigui_tree_cb_button_press), tree);

    /* enable tree drag and drop */
    gtk_tree_view_enable_model_drag_dest(GTK_TREE_VIEW(treeview),
                                         target_table, G_N_ELEMENTS(target_table),
                                         GDK_ACTION_COPY);
    gtk_tree_view_enable_model_drag_source(GTK_TREE_VIEW(treeview),
                                           GDK_BUTTON1_MASK, target_table,
                                           G_N_ELEMENTS(target_table),
                                           GDK_ACTION_COPY);
    g_signal_connect(G_OBJECT(treeview), "drag-data-received",
                     G_CALLBACK(swamigui_tree_cb_drag_data_received), tree);
    g_signal_connect(G_OBJECT(treeview), "drag-data-get",
                     G_CALLBACK(swamigui_tree_cb_drag_data_get), tree);

    *out_treeview = treeview;

    return (scrollwin);
}

/* cell renderer for tree pixmap icons */
static void
swamigui_tree_item_icon_cell_data(GtkTreeViewColumn *tree_column,
                                  GtkCellRenderer *cell,
                                  GtkTreeModel *tree_model,
                                  GtkTreeIter *iter,
                                  gpointer data)
{
    GtkTreeView *treeview = GTK_TREE_VIEW(data);
    GdkPixbuf *icon = NULL;
    char *stock_id;

    gtk_tree_model_get(tree_model, iter,
                       SWAMIGUI_TREE_STORE_ICON_COLUMN, &stock_id,
                       -1);

    if(stock_id)		/* ++ ref new closed pixmap */
        icon = gtk_widget_render_icon(GTK_WIDGET(treeview), stock_id,
                                      GTK_ICON_SIZE_SMALL_TOOLBAR, NULL);

    g_object_set(cell, "pixbuf", icon, NULL);

    if(icon)
    {
        g_object_unref(icon);    /* -- unref icon */
    }
}

/* cell renderer for tree labels (overridden to allow for search highlighting) */
static void
swamigui_tree_item_label_cell_data(GtkTreeViewColumn *tree_column,
                                   GtkCellRenderer *cell,
                                   GtkTreeModel *tree_model,
                                   GtkTreeIter *iter,
                                   gpointer data)
{
    SwamiguiTree *tree = SWAMIGUI_TREE(data);
    PangoAttrList *alist = NULL;
    PangoAttribute *attr;
    char *label;
    GObject *obj;

    gtk_tree_model_get(tree_model, iter,
                       SWAMIGUI_TREE_STORE_LABEL_COLUMN, &label,	/* ++ alloc */
                       SWAMIGUI_TREE_STORE_OBJECT_COLUMN, &obj,	/* ++ ref */
                       -1);

    /* should this object be highlighted for in progress search? */
    if(tree->search_match == obj)
    {
        alist = pango_attr_list_new();

        attr = pango_attr_background_new(0, 65535, 0);
        attr->start_index = tree->search_start_pos;
        attr->end_index = tree->search_end_pos;
        pango_attr_list_insert(alist, attr);
    }

    g_object_set(cell, "text", label, "attributes", alist, NULL);

    if(alist)
    {
        pango_attr_list_unref(alist);
    }

    g_object_unref(obj);	/* -- unref */
    g_free(label);	/* -- free */
}

/* a callback for when the tree view selection changes */
static void
tree_cb_selection_changed(GtkTreeSelection *selection, SwamiguiTree *tree)
{
    GtkTreeView *treeview;

    treeview = gtk_tree_selection_get_tree_view(selection);

    /* update currently selected GtkTreeView and store */
    tree->seltree = treeview;
    tree->selstore = SWAMIGUI_TREE_STORE(gtk_tree_view_get_model(treeview));

    /* update the current selection to that of the current selected tree */
    swamigui_tree_update_selection(tree);
}

/* when a tree view gets a button press */
static gboolean
swamigui_tree_cb_button_press(GtkWidget *widg, GdkEventButton *event,
                              SwamiguiTree *tree)
{
    GtkTreeSelection *tree_sel;
    GtkTreeModel *model;
    GtkTreePath *path;
    GtkTreeIter iter;
    GObject *rclick_item;
    int x, y;

    if(!tree->seltree)
    {
        return (FALSE);    /* Shouldn't happen, but.. */
    }

    if(event->button != 3)
    {
        return (FALSE);
    }

    x = (int)event->x;		/* x and y coordinates are of type double */
    y = (int)event->y;		/* convert to integer */

    /* get the tree path at the given mouse cursor position
     * ++ alloc path */
    gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(tree->seltree),
                                  x, y, &path, NULL, NULL, NULL);

    if(!path)
    {
        return (FALSE);
    }

    /* right click */
    model = gtk_tree_view_get_model(GTK_TREE_VIEW(tree->seltree));

    /* convert path to iter */
    if(!gtk_tree_model_get_iter(model, &iter, path))
    {
        gtk_tree_path_free(path);	/* -- free path */
        return (FALSE);
    }

    gtk_tree_path_free(path);	/* -- free path */

    /* stop button press event propagation */
    gtk_signal_emit_stop_by_name(GTK_OBJECT(widg), "button-press-event");

    rclick_item =
        swamigui_tree_store_node_get_item(SWAMIGUI_TREE_STORE(model), &iter);

    /* check if right click is not part of current selection */
    if(!tree->selection || !g_list_find(tree->selection->items, rclick_item))
    {
        /* click not on selection, clear it and select the single item */
        tree_sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree->seltree));

        gtk_tree_selection_unselect_all(tree_sel);
        gtk_tree_selection_select_iter(tree_sel, &iter);

        tree_cb_selection_changed(tree_sel, tree);	/* update selection */
    }

    /* do the menu popup */
    swamigui_tree_do_popup_menu(tree, rclick_item, event);

    return (TRUE);
}

static void
swamigui_tree_do_popup_menu(SwamiguiTree *tree, GObject *rclick_item,
                            GdkEventButton *event)
{
    SwamiguiItemMenu *menu;
    int button, event_time;

    if(!tree->selection)
    {
        return;    /* No selection, no menu */
    }

    menu = swamigui_item_menu_new();
    g_object_set(menu,
                 "selection", tree->selection,
                 "right-click", rclick_item,
                 "creator", tree,
                 NULL);

    swamigui_item_menu_generate(menu);

    if(event)
    {
        button = event->button;
        event_time = event->time;
    }
    else
    {
        button = 0;
        event_time = gtk_get_current_event_time();
    }

    gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL,
                   button, event_time);
}

/* callback for when drag data received */
static void
swamigui_tree_cb_drag_data_received(GtkWidget *widget, GdkDragContext *context,
                                    gint x, gint y,
                                    GtkSelectionData *selection_data,
                                    guint info, guint time, gpointer data)
{
    SwamiguiTree *tree = SWAMIGUI_TREE(data);
    SwamiguiTreeStore *store;
    GtkTreeView *treeview = GTK_TREE_VIEW(widget);
    char *atomname;

    if(selection_data->format != 8 ||
            selection_data->length == 0)
    {
        g_critical("DND on Swami tree had invalid format (%d) or length (%d)",
                   selection_data->format,
                   selection_data->length);
        return;
    }

    atomname = gdk_atom_name(selection_data->type);

    /* drag and drop between or within tree? */
    if(strcmp(atomname, SWAMIGUI_DND_OBJECT_NAME) == 0)
    {
        GtkTreePath *path;
        GtkTreeIter iter;
        IpatchItem *destitem;
        int itemcount, pastecount;
        IpatchList *objlist;
        GList *p;

        objlist = *((IpatchList **)(selection_data->data));

        /* make sure source object is an IpatchList (could be another object type) */
        if(!IPATCH_IS_LIST(objlist))
        {
            return;
        }

        /* get path to destination drop row */
        if(!gtk_tree_view_get_path_at_pos(treeview, x, y, &path, NULL, NULL, NULL))
        {
            return;    /* return if no row at drop position */
        }

        /* get iterator for path */
        if(!gtk_tree_model_get_iter(gtk_tree_view_get_model(treeview), &iter,
                                    path))
        {
            gtk_tree_path_free(path);
            return;
        }

        gtk_tree_path_free(path);

        store = swamigui_tree_get_selected_store(tree);
        destitem = IPATCH_ITEM(swamigui_tree_store_node_get_item(store, &iter));

        if(!destitem)
        {
            return;
        }

        /* loop over source items */
        for(p = objlist->items, itemcount = 0, pastecount = 0; p;
                p = p->next, itemcount++)
        {
            if(IPATCH_ITEM(p->data))
            {
                if(ipatch_simple_paste(destitem, (IpatchItem *)(p->data), NULL))
                {
                    pastecount++;
                }
            }
        }

        if(itemcount > 0)
        {
            if(itemcount != pastecount)
                swamigui_statusbar_printf(swamigui_root->statusbar,
                                          _("Pasted %d of %d item(s)"), pastecount,
                                          itemcount);
            else
                swamigui_statusbar_printf(swamigui_root->statusbar,
                                          _("Pasted %d item(s)"), itemcount);
        }
    }	/* drop of external file URI on tree */
    else if(strcmp(atomname, SWAMIGUI_DND_URI_NAME) == 0)
    {
        char *uri_list;
        char **uris;
        char *fname;
        GError *err = NULL;
        int i;

        uri_list = g_strndup((char *)(selection_data->data),
                             selection_data->length);

        uris = g_strsplit(uri_list, "\r\n", 0);

        /* loop over URIs and attempt to open any that are file names */
        for(i = 0; uris && uris[i]; i++)
        {
            fname = g_filename_from_uri(uris[i], NULL, NULL);

            if(fname)
            {
                /* loading patch file and log error on Gtk message dialog */
                swamigui_root_patch_load(swami_root, fname, NULL,
                                         GTK_WINDOW(swamigui_root->main_window));

                g_free(fname);
            }
        }

        g_strfreev(uris);
        g_free(uri_list);
    }
}

static void
swamigui_tree_cb_drag_data_get(GtkWidget *widget, GdkDragContext *drag_context,
                               GtkSelectionData *data, guint info,
                               guint time, gpointer user_data)
{
    SwamiguiTree *tree = SWAMIGUI_TREE(user_data);
    GdkAtom atom;

#if GTK_CHECK_VERSION (2, 10, 0)
    atom = gdk_atom_intern_static_string(SWAMIGUI_DND_OBJECT_NAME);
#else
    atom = gdk_atom_intern(SWAMIGUI_DND_OBJECT_NAME, FALSE);
#endif

    gtk_selection_data_set(data, atom, 8, (guint8 *)(&tree->selection),
                           sizeof(IpatchList *));
}

/* callback for when tree search entry text changes */
static void
swamigui_tree_cb_search_entry_changed(GtkEntry *entry, gpointer user_data)
{
    SwamiguiTree *tree = SWAMIGUI_TREE(user_data);
    const char *search;

    search =  gtk_entry_get_text(entry);	/* get current search string */
    swamigui_tree_search_set_text(tree, search);
}

static void
swamigui_tree_cb_search_next_clicked(GtkButton *button, gpointer user_data)
{
    SwamiguiTree *tree = SWAMIGUI_TREE(user_data);
    swamigui_tree_search_next(tree);
}

static void
swamigui_tree_cb_search_prev_clicked(GtkButton *button, gpointer user_data)
{
    SwamiguiTree *tree = SWAMIGUI_TREE(user_data);
    swamigui_tree_search_prev(tree);
}

/**
 * swamigui_tree_get_store_list:
 * @tree: Swami GUI tree view
 *
 * Gets the tree stores of a tree view.
 *
 * Returns: List of #SwamiguiTreeStore objects or %NULL if none, NO
 * reference is added and so the list should only be used within the
 * context of the calling function unless referenced or duplicated.
 */
IpatchList *
swamigui_tree_get_store_list(SwamiguiTree *tree)
{
    g_return_val_if_fail(SWAMIGUI_IS_TREE(tree), NULL);
    return (tree->stores);
}

/**
 * swamigui_tree_set_selected_store:
 * @tree: Swami tree object
 * @store: Store to select as the active store
 *
 * Sets the currently selected store.  The notebook tab containing @store will
 * be selected which will cause the current tree selection to be updated to the
 * item selection of the GtkTreeView contained therein.
 */
void
swamigui_tree_set_selected_store(SwamiguiTree *tree,
                                 SwamiguiTreeStore *store)
{
    int store_index;

    g_return_if_fail(SWAMIGUI_IS_TREE(tree));
    g_return_if_fail(SWAMIGUI_IS_TREE_STORE(store));
    g_return_if_fail(tree->stores != NULL);

    store_index = g_list_index(tree->stores->items, store);
    g_return_if_fail(store_index != -1);

    gtk_notebook_set_current_page(tree->notebook, store_index);
}

/**
 * swamigui_tree_get_selected_store:
 * @tree: Swami tree object
 *
 * Get the currently selected tree store (displayed in the current notebook tab).
 *
 * Returns: Currently selected tree store or %NULL if none.  Not referenced and
 * so caller should take care to reference it if using outside of calling scope.
 */
SwamiguiTreeStore *
swamigui_tree_get_selected_store(SwamiguiTree *tree)
{
    g_return_val_if_fail(SWAMIGUI_IS_TREE(tree), NULL);
    return (tree->selstore);
}

/**
 * swamigui_tree_get_selection_single:
 * @tree: Swami tree object
 *
 * Get and insure single item selection in Swami tree object
 *
 * Returns: The currently selected single item or %NULL if multiple or no items
 *   are selected. A reference is not added so caller should take care to
 *   reference it if using outside of the calling scope.
 */
GObject *
swamigui_tree_get_selection_single(SwamiguiTree *tree)
{
    GList *sel = NULL;

    g_return_val_if_fail(SWAMIGUI_IS_TREE(tree), NULL);

    if(tree->selection)
    {
        sel = tree->selection->items;
    }

    if(sel && !sel->next)
    {
        return (G_OBJECT(sel->data));
    }
    else
    {
        return (NULL);
    }
}

/**
 * swamigui_tree_get_selection:
 * @tree: Swami tree object
 *
 * Get Swami tree selection.
 *
 * Returns: List of items in the tree selection or %NULL if no items
 * selected. The returned list object is internal and should not be
 * modified.
 *
 * NOTE - The list's reference count is not incremented, so if it is
 * desirable to use the list beyond the scope of the calling function
 * it should be duplicated or a reference added.
 */
IpatchList *
swamigui_tree_get_selection(SwamiguiTree *tree)
{
    g_return_val_if_fail(SWAMIGUI_IS_TREE(tree), NULL);
    return (tree->selection);
}

/**
 * swamigui_tree_clear_selection:
 * @tree: Swami tree object
 *
 * Clear tree selection (unselect all items)
 */
void
swamigui_tree_clear_selection(SwamiguiTree *tree)
{
    GtkTreeSelection *sel;

    g_return_if_fail(SWAMIGUI_IS_TREE(tree));

    if(!tree->seltree)
    {
        return;
    }

    sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree->seltree));
    gtk_tree_selection_unselect_all(sel);
}

/**
 * swamigui_tree_set_selection:
 * @tree: Swami tree object
 * @list: List of objects to select
 *
 * Set the tree selection.  List of objects must be in the same tree store
 * (notebook tab).  If items are in a non selected store it will become
 * selected.
 */
void
swamigui_tree_set_selection(SwamiguiTree *tree, IpatchList *list)
{
    swamigui_tree_set_selection_real(tree, list, 3);
}

/* the real selection set function, notify_flags 1 << 0 = "selection" and
   1 << 1 = "selection-single" */
static void
swamigui_tree_set_selection_real(SwamiguiTree *tree, IpatchList *list,
                                 int notify_flags)
{
    GtkTreeModel *model;
    GtkTreeSelection *selection;
    GtkTreePath *path, *firstpath = NULL;
    GtkTreeView *seltree;
    GtkTreeIter treeiter, parent;
    gboolean new_sel_single;
    GObject *item;
    GList *p, *foundstore;
    int pos;

    g_return_if_fail(SWAMIGUI_IS_TREE(tree));
    g_return_if_fail(!list || IPATCH_IS_LIST(list));

    if(!tree->stores)
    {
        return;    /* no point in selecting nothing, but.. */
    }

    /* list contains items? */
    if(list && list->items)
    {
        /* first item shall suffice (all should be present and in same store) */
        item = G_OBJECT(list->items->data);

        /* locate the store containing the first item */
        for(p = tree->stores->items, pos = 0; p; p = p->next, pos++)
            if(swamigui_tree_store_item_get_node(SWAMIGUI_TREE_STORE(p->data),
                                                 item, NULL))
            {
                break;
            }

        foundstore = p;

        if(swami_log_if_fail(foundstore))
        {
            return;
        }

        if(tree->selstore != p->data)	/* selected store has changed? */
        {
            tree->selstore = SWAMIGUI_TREE_STORE(p->data);
            tree->seltree = g_list_nth_data(tree->treeviews, pos);

            /* switch the page without signaling our switch page handler */
            g_signal_handlers_block_by_func(tree, swamigui_tree_cb_switch_page, NULL);
            gtk_notebook_set_current_page(tree->notebook, pos);
            g_signal_handlers_unblock_by_func(tree, swamigui_tree_cb_switch_page, NULL);
        }
    }		/* no items in new list */
    else if(!tree->selstore)
    {
        return;    /* no selected store? - return */
    }

    if(tree->selection)
    {
        g_object_unref(tree->selection);    /* -- unref old selection */
    }

    if(list)
    {
        tree->selection = ipatch_list_duplicate(list);
    }
    else
    {
        tree->selection = ipatch_list_new();
    }

    /* set the object's origin so swamigui_root can report this to others */
    swami_object_set_origin(G_OBJECT(tree->selection), G_OBJECT(tree));

    model = gtk_tree_view_get_model(GTK_TREE_VIEW(tree->seltree));
    selection = gtk_tree_view_get_selection(tree->seltree);

    g_signal_handlers_block_by_func(selection,
                                    G_CALLBACK(tree_cb_selection_changed),
                                    tree);

    gtk_tree_selection_unselect_all(selection);

    seltree = GTK_TREE_VIEW(tree->seltree);

    /* update the tree view selection and expand all parents of items */
    for(p = list ? list->items : NULL; p; p = p->next)
    {
        if(swamigui_tree_store_item_get_node(SWAMIGUI_TREE_STORE(model),
                                             p->data, &treeiter))
        {
            if(!firstpath)	/* ++ alloc path (for first valid item) */
            {
                firstpath = gtk_tree_model_get_path(model, &treeiter);
            }

            /* expand all parents as necessary */
            if(gtk_tree_model_iter_parent(model, &parent, &treeiter))
            {
                path = gtk_tree_model_get_path(model, &parent);	/* ++ alloc path */
                gtk_tree_view_expand_to_path(seltree, path);
                gtk_tree_path_free(path);	/* -- free path */
            }

            /* select item in tree selection */
            gtk_tree_selection_select_iter(selection, &treeiter);
        }
    }

    g_signal_handlers_unblock_by_func(selection,
                                      G_CALLBACK(tree_cb_selection_changed),
                                      tree);

    /* spotlight the first item in the selection */
    if(firstpath)
    {
        gtk_tree_view_scroll_to_cell(seltree, firstpath, NULL, FALSE, 0.0, 0.0);
        gtk_tree_path_free(firstpath);	/* -- free path */
    }

    /* notify single selection change if old and new single are not NULL */
    new_sel_single = list && list->items && !list->items->next;

    if((notify_flags & 2) && (new_sel_single || tree->sel_single))
    {
        g_object_notify(G_OBJECT(tree), "selection-single");
    }

    tree->sel_single = new_sel_single;

    if(notify_flags & 1)
    {
        g_object_notify(G_OBJECT(tree), "selection");
    }
}

/**
 * swamigui_tree_spotlight_item:
 * @tree: Swami tree object
 * @item: Object in @tree to spotlight
 *
 * Spotlights an item in a Swami tree object by recursively expanding all
 * nodes up the tree from item and moving the view to position item in the
 * center of the view and then selects it.  If the item is part of an unselected
 * store (i.e., notebook tab), then it will become selected.
 */
void
swamigui_tree_spotlight_item(SwamiguiTree *tree, GObject *item)
{
    GtkTreeModel *model;
    GtkTreeSelection *sel;
    GtkTreeIter iter;
    GtkTreePath *path;
    GList *p, *foundstore;
    int pos;

    g_return_if_fail(SWAMIGUI_IS_TREE(tree));
    g_return_if_fail(G_IS_OBJECT(item));

    /* locate the store containing the first item */
    for(p = tree->stores->items, pos = 0; p; p = p->next, pos++)
        if(swamigui_tree_store_item_get_node(SWAMIGUI_TREE_STORE(p->data),
                                             item, NULL))
        {
            break;
        }

    foundstore = p;

    if(swami_log_if_fail(foundstore))
    {
        return;
    }

    /* store is not selected store? */
    if((SwamiguiTreeStore *)(p->data) != tree->selstore)
    {
        tree->selstore = SWAMIGUI_TREE_STORE(p->data);
        tree->seltree = g_list_nth_data(tree->treeviews, pos);

        /* switch the page without signaling our switch page handler */
        g_signal_handlers_block_by_func(tree, swamigui_tree_cb_switch_page, NULL);
        gtk_notebook_set_current_page(tree->notebook, pos);
        g_signal_handlers_unblock_by_func(tree, swamigui_tree_cb_switch_page, NULL);
    }

    model = gtk_tree_view_get_model(GTK_TREE_VIEW(tree->seltree));

    if(!swamigui_tree_store_item_get_node(SWAMIGUI_TREE_STORE(model),
                                          item, &iter))
    {
        return;
    }

    sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree->seltree));

    /* expand the nodes parents */
    path = gtk_tree_model_get_path(model, &iter);

    if(gtk_tree_path_up(path))
    {
        gtk_tree_view_expand_to_path(GTK_TREE_VIEW(tree->seltree), path);
    }

    gtk_tree_path_free(path);

    /* scroll to the given item */
    path = gtk_tree_model_get_path(model, &iter);
    gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(tree->seltree), path, NULL,
                                 TRUE, 0.5, 0.0);
    gtk_tree_path_free(path);

    /* select the item */
    gtk_tree_selection_unselect_all(sel);
    gtk_tree_selection_select_iter(sel, &iter);
}


/**
 * swamigui_tree_search_set_start:
 * @tree: Tree widget
 * @start: Start object to begin search from (inclusive, %NULL for entire tree)
 *
 * Sets the beginning object in tree to start searching from, the passed
 * object is included in the search (can match).
 */
void
swamigui_tree_search_set_start(SwamiguiTree *tree, GObject *start)
{
    g_return_if_fail(SWAMIGUI_IS_TREE(tree));
    g_return_if_fail(!start);	/* we don't actually care if it is valid */

    tree->search_start = start;
}

/**
 * swamigui_tree_search_set_text:
 * @tree: Tree widget
 * @text: Text to set search to
 *
 * Set the text of the tree's search entry and update search selection.
 */
void
swamigui_tree_search_set_text(SwamiguiTree *tree, const char *text)
{
    g_return_if_fail(SWAMIGUI_IS_TREE(tree));

    g_free(tree->search_text);
    tree->search_text = g_strdup(text);
    swamigui_tree_real_search_next(tree, FALSE);
}

/**
 * swamigui_tree_search_set_visible:
 * @tree: Tree widget
 * @active: TRUE to show search entry, FALSE to hide it
 *
 * Shows/hides the search entry below the tree.
 */
void
swamigui_tree_search_set_visible(SwamiguiTree *tree, gboolean visible)
{
    g_return_if_fail(SWAMIGUI_IS_TREE(tree));

    if(visible)
    {
        gtk_widget_show(tree->search_box);
    }
    else
    {
        gtk_widget_hide(tree->search_box);
    }
}

/**
 * swamigui_tree_search_next:
 * @tree: Tree widget
 *
 * Go to the next matching item for the current search.
 */
void
swamigui_tree_search_next(SwamiguiTree *tree)
{
    swamigui_tree_real_search_next(tree, TRUE);
}

static void
swamigui_tree_real_search_next(SwamiguiTree *tree, gboolean usematch)
{
    GtkTreeModel *model;
    GtkTreeIter iter;
    char *label;
    GObject *obj;
    int index;

    g_return_if_fail(SWAMIGUI_IS_TREE(tree));

    if(!tree->selstore || !tree->seltree)
    {
        return;    /* FIXME: silently fail? */
    }

    model = GTK_TREE_MODEL(tree->selstore);

    /* if search_match is set and valid, set start iter to the next node thereof */
    if(usematch && tree->search_match && swamigui_tree_store_item_get_node
            (tree->selstore, tree->search_match, &iter))
    {
        tree_iter_recursive_next(model, &iter);
    }
    else	/* no search match item (or !usematch), try search start */
    {
        if(!tree->search_start || !swamigui_tree_store_item_get_node
                (tree->selstore, tree->search_start, &iter))
        {
            /* no search start item, try first item in tree */
            if(!gtk_tree_model_get_iter_first(model, &iter))
            {
                return;    /* empty tree? - return */
            }

            gtk_tree_model_get(model, &iter,
                               SWAMIGUI_TREE_STORE_OBJECT_COLUMN, &obj,	/* ++ ref */
                               -1);
            tree->search_start = obj;
            g_object_unref(obj);	/* -- unref */
        }
    }

    /* iterate over tree looking for matching item */
    do
    {
        gtk_tree_model_get(model, &iter,
                           SWAMIGUI_TREE_STORE_LABEL_COLUMN, &label,	/* ++ alloc */
                           SWAMIGUI_TREE_STORE_OBJECT_COLUMN, &obj,	/* ++ ref */
                           -1);

        /* search for sub string in row label */
        index = str_index(label, tree->search_text);
        g_free(label);		/* -- free */

        if(index >= 0)	/* matched? */
        {
            set_search_match_item(tree, &iter, obj, index, tree->search_text);
            g_object_unref(obj);	/* -- unref */
            return;	/* we done */
        }

        g_object_unref(obj);	/* -- unref */
    }
    while(tree_iter_recursive_next(model, &iter));

    reset_search_match_item(tree, NULL);	/* no match, nothing selected */
}

/**
 * swamigui_tree_search_next:
 * @tree: Tree widget
 *
 * Go to the previous matching item for the current search.
 */
void
swamigui_tree_search_prev(SwamiguiTree *tree)
{
    GtkTreeModel *model;
    GtkTreeIter iter, current;
    char *label;
    GObject *obj;
    int index;

    g_return_if_fail(SWAMIGUI_IS_TREE(tree));

    if(!tree->selstore || !tree->seltree)
    {
        return;    /* FIXME: silently fail? */
    }

    model = GTK_TREE_MODEL(tree->selstore);

    /* if search_match is set and valid, set start iter to the prev node thereof */
    if(tree->search_match && swamigui_tree_store_item_get_node
            (tree->selstore, tree->search_match, &iter))
    {
        if(!tree_iter_recursive_prev(model, &iter))
        {
            return;    /* FIXME - Wrap around? */
        }
    }
    else	/* no search match item, try search start */
    {
        if(!tree->search_start || !swamigui_tree_store_item_get_node
                (tree->selstore, tree->search_start, &iter))
        {
            /* no search start, find last item of tree */
            if(!gtk_tree_model_get_iter_first(model, &iter))
            {
                return;    /* empty tree? - return */
            }

            /* find last child of last sibling of tree */
            do
            {
                /* find last sibling at this level */
                current = iter;

                while(gtk_tree_model_iter_next(model, &current))
                {
                    iter = current;
                }

                current = iter;
            }
            while(gtk_tree_model_iter_children(model, &iter, &current));

            iter = current;

            gtk_tree_model_get(model, &iter,
                               SWAMIGUI_TREE_STORE_OBJECT_COLUMN, &obj,	/* ++ ref */
                               -1);
            tree->search_start = obj;
            g_object_unref(obj);	/* -- unref */
        }
    }

    /* iterate over tree looking for matching item */
    do
    {
        gtk_tree_model_get(model, &iter,
                           SWAMIGUI_TREE_STORE_LABEL_COLUMN, &label,	/* ++ alloc */
                           SWAMIGUI_TREE_STORE_OBJECT_COLUMN, &obj,	/* ++ ref */
                           -1);

        /* search for sub string in row label */
        index = str_index(label, tree->search_text);
        g_free(label);		/* -- free */

        if(index >= 0)	/* matched? */
        {
            set_search_match_item(tree, &iter, obj, index, tree->search_text);
            g_object_unref(obj);	/* -- unref */
            return;	/* we done */
        }

        g_object_unref(obj);	/* -- unref */
    }
    while(tree_iter_recursive_prev(model, &iter));

    reset_search_match_item(tree, NULL);	/* no match, nothing selected */
}

static void
set_search_match_item(SwamiguiTree *tree, GtkTreeIter *iter, GObject *obj,
                      int startpos, const char *search)
{
    GtkTreeModel *store;
    GtkTreePath *path, *parent;
    GtkTreeIter myiter, piter;
    GList *new_ancestry = NULL;
    GObject *pobj;
    GList *p;

    if(!tree->selstore || !tree->seltree)
    {
        return;
    }

    store = GTK_TREE_MODEL(tree->selstore);

    if(!iter)	/* if only the object was passed, lookup tree iter */
    {
        iter = &myiter;

        if(!swamigui_tree_store_item_get_node(tree->selstore, obj, iter))
        {
            return;
        }
    }

    path = gtk_tree_model_get_path(store, iter);	/* ++ alloc */
    parent = gtk_tree_path_copy(path);		/* ++ alloc */

    /* create ancestry list of new match item */
    while(gtk_tree_path_up(parent) && gtk_tree_path_get_depth(parent) > 0)
    {
        gtk_tree_model_get_iter(store, &piter, parent);
        gtk_tree_model_get(store, &piter,
                           SWAMIGUI_TREE_STORE_OBJECT_COLUMN, &pobj, /* ++ ref */
                           -1);
        new_ancestry = g_list_prepend(new_ancestry, pobj);
        g_object_unref(pobj);	/* -- unref */
    }

    /* previous match and not the same? - reset it */
    if(tree->search_match && tree->search_match != obj)
    {
        reset_search_match_item(tree, &new_ancestry);
    }

    /* set the search object and start/end text position */
    tree->search_match = obj;
    tree->search_start_pos = startpos;
    tree->search_end_pos = startpos + strlen(search);

    /* Tell the model that the row display should be updated.
     * GtkTreeCellDataFunc will handle the highlight of the label. */
    gtk_tree_model_row_changed(store, path, iter);

    /* loop on remaining ancestry items (that weren't already in search_expanded) */
    for(p = new_ancestry; p; p = g_list_delete_link(p, p))
    {
        /* probably won't fail, but.. */
        if(swamigui_tree_store_item_get_node(tree->selstore, p->data, &piter))
        {
            parent = gtk_tree_model_get_path(store, &piter);	/* ++ alloc */

            /* not expanded? - Add to search_expanded list */
            if(!gtk_tree_view_row_expanded(tree->seltree, parent))
            {
                tree->search_expanded = g_list_prepend(tree->search_expanded, p->data);
            }

            gtk_tree_path_free(parent);	/* -- free */
        }
    }

    /* expand all ancestry of the matched node */
    parent = gtk_tree_path_copy(path);	/* ++ alloc */

    if(gtk_tree_path_up(parent))
    {
        gtk_tree_view_expand_to_path(tree->seltree, parent);
    }

    gtk_tree_path_free(parent);	/* -- free */

    /* scroll if needed so the highlighted item is in view */
    gtk_tree_view_scroll_to_cell(tree->seltree, path, NULL, FALSE, 0.0, 0.0);

    gtk_tree_path_free(path);	/* -- free */
}

/* resets the current search match item.
 * new_ancestry is optional and specifies GObject ancestry of a new item which
 * will become selected.  Nodes shared between the old search_expanded list
 * and the new_ancestry list are not collapsed, not removed from old list and
 * removed from new list.  Nodes in old list which are not in new list are
 * collapsed and removed from list.  Nodes in new list not in old list are
 * left alone.  This allows for branches to remain open which will be part of
 * a new match item (therefore the scroll to the new item will only be done if
 * necessary).
 */
static void
reset_search_match_item(SwamiguiTree *tree, GList **new_ancestry)
{
    GtkTreeIter iter;
    GtkTreePath *path;
    gboolean retval;
    GList *p, *tmp, *match;
    GObject *obj;

    if(!tree->search_match || !tree->selstore || !tree->seltree)
    {
        return;
    }

    /* get the tree node for the current match item */
    retval = swamigui_tree_store_item_get_node(tree->selstore,
             tree->search_match, &iter);
    tree->search_match = NULL;

    if(!retval)
    {
        return;    /* can happen if search match gets removed */
    }

    /* notify that item row has changed and should be updated (unhighlighted) */
    path = gtk_tree_model_get_path(GTK_TREE_MODEL(tree->selstore), &iter);
    gtk_tree_model_row_changed(GTK_TREE_MODEL(tree->selstore), path, &iter);
    gtk_tree_path_free(path);

    /* collapse any parents that were expanded and delete list */
    for(p = tree->search_expanded; p;)
    {
        obj = (GObject *)(p->data);

        if(new_ancestry)	/* new ancestry list provided? */
        {
            /* check if object is in new list also */
            if((match = g_list_find(*new_ancestry, obj)))
            {
                /* found in new list: remove from new list, leave in old list */
                *new_ancestry = g_list_delete_link(*new_ancestry, match);
                p = p->next;
                continue;
            }
        }

        /* delete from old list */
        tmp = p;
        p = p->next;
        tree->search_expanded = g_list_delete_link(tree->search_expanded, tmp);

        if(!swamigui_tree_store_item_get_node(tree->selstore, obj, &iter))
        {
            continue;
        }

        path = gtk_tree_model_get_path(GTK_TREE_MODEL(tree->selstore), &iter);
        gtk_tree_view_collapse_row(GTK_TREE_VIEW(tree->seltree), path);
        gtk_tree_path_free(path);
    }
}

/* search for substring case insensitively and return the index of the match
 * or -1 on no match */
static int
str_index(const char *haystack, const char *needle)
{
    const char *s1, *s2;
    int i;

    if(!haystack || !needle)
    {
        return -1;
    }

    for(i = 0; *haystack; haystack++, i++)
    {
        for(s1 = haystack, s2 = needle;
                toupper(*s1) == toupper(*s2) && *s1 && *s2;
                s1++, s2++);

        if(!*s2)
        {
            return (i);
        }
    }

    return (-1);
}

/* For recursing forwards through tree one node at a time */
static gboolean
tree_iter_recursive_next(GtkTreeModel *model, GtkTreeIter *iter)
{
    GtkTreeIter current = *iter;
    GtkTreeIter parent;

    /* attempt to go to first child of current node */
    if(gtk_tree_model_iter_children(model, iter, &current))
    {
        return (TRUE);
    }

    /* attempt to go to next sibling of current node */
    *iter = current;

    if(gtk_tree_model_iter_next(model, iter))
    {
        return (TRUE);
    }

    /* attempt to go to next possible sibling of the closest parent */
    while(TRUE)
    {
        if(!gtk_tree_model_iter_parent(model, &parent, &current))
        {
            return (FALSE);
        }

        *iter = parent;

        if(gtk_tree_model_iter_next(model, iter))
        {
            return (TRUE);
        }

        current = parent;
    }
}

/* For recursing backwards through tree one node at a time */
static gboolean
tree_iter_recursive_prev(GtkTreeModel *model, GtkTreeIter *iter)
{
    GtkTreeIter current;
    GtkTreePath *path;

    /* get a path, since it is easier to work with for some operations (such a prev) */
    path = gtk_tree_model_get_path(model, iter);		/* ++ alloc */

    /* attempt to go to previous sibling */
    if(gtk_tree_path_prev(path))
    {
        gtk_tree_model_get_iter(model, iter, path);	/* shouldn't fail */
        gtk_tree_path_free(path);	/* -- free */

        /* attempt to recursively go to last child */
        while(gtk_tree_model_iter_children(model, &current, iter))
        {
            /* find last sibling */
            *iter = current;

            while(gtk_tree_model_iter_next(model, &current))
            {
                *iter = current;
            }
        }

        /* at this point, its either previous sibling with no children or
         * deepest last child of previous sibling */
        return (TRUE);
    }

    gtk_tree_path_free(path);	/* -- free */

    /* attempt to go to parent */
    current = *iter;

    if(gtk_tree_model_iter_parent(model, iter, &current))
    {
        return (TRUE);
    }

    return (FALSE);	/* that was the topmost node */
}
