/*
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
#include <gtk/gtk.h>
#include <string.h>
#include <gdk/gdkkeysyms.h>

#include "SwamiguiPref.h"
#include "SwamiguiControl.h"
#include "SwamiguiRoot.h"
#include "util.h"
#include "icons.h"
#include "i18n.h"

/* columns for preference sections list store */
enum
{
    SECTIONS_COLUMN_ICON,
    SECTIONS_COLUMN_NAME,
    SECTIONS_COLUMN_COUNT
};

/* columns for piano key binding list store */
enum
{
    KEYBIND_COLUMN_NOTE,
    KEYBIND_COLUMN_KEY,
    KEYBIND_COLUMN_COUNT
};

/* Action used for piano key binding capture */
typedef enum
{
    BIND_MODE_INACTIVE,
    BIND_MODE_ADD,
    BIND_MODE_CHANGE
} BindMode;

/* Stores information on a registered preferences interface */
typedef struct
{
    char *icon;			/* Stock icon of preferences interface */
    char *name;			/* Name of the interface */
    int order;			/* Sort order for this preference interface */
    SwamiguiPrefHandler handler;	/* Handler to create the interface */
} PrefInfo;

static gint sort_pref_info_by_order_and_name(gconstpointer a, gconstpointer b);
static void swamigui_pref_cb_close_clicked(GtkWidget *button, gpointer user_data);
static void swamigui_pref_section_list_change(GtkTreeSelection *selection,
        gpointer user_data);
static GtkWidget *general_prefs_handler(void);
static GtkWidget *audio_samples_prefs_handler(void);
static GtkWidget *keyboard_prefs_handler(void);
static void keybindings_update(GtkWidget *prefwidg, gboolean lower);
static void keybindings_set_bind_mode(GtkWidget *prefwidg, BindMode mode);
static void keybindings_lower_radio_toggled(GtkToggleButton *btn,
        gpointer user_data);
static void keybindings_add_key_button_toggled(GtkToggleButton *btn,
        gpointer user_data);
static void keybindings_change_key_button_toggled(GtkToggleButton *btn,
        gpointer user_data);
static void keybindings_delete_key_button_clicked(GtkButton *btn, gpointer user_data);
static void keybindings_reset_keys_button_clicked(GtkButton *btn, gpointer user_data);
static void keybindings_reset_keys_response(GtkDialog *dialog, int response,
        gpointer user_data);
static void keybindings_selection_foreach(GtkTreeModel *model, GtkTreePath *path,
        GtkTreeIter *iter, gpointer data);
static gboolean keybindings_key_press_event(GtkWidget *widg, GdkEventKey *event,
        gpointer user_data);
static void keybindings_sync(GtkWidget *prefwidg);

G_DEFINE_TYPE(SwamiguiPref, swamigui_pref, GTK_TYPE_DIALOG);


static GList *pref_list = NULL;	/* list of registered pref interfaces (PrefInfo *) */

#define swamigui_pref_info_new()	g_slice_new (PrefInfo)

static const char *note_names[] =
{ "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };


/**
 * swamigui_register_pref_handler:
 * @name: Name of the preference interface (shown in list with icon)
 * @icon: Name of stock icon for interface (shown in list with name)
 * @order: Order of the interface in relation to others (the lower the number
 *   the higher on the list, use #SWAMIGUI_PREF_ORDER_NAME to indicate that the
 *   interface should be sorted by name - after other interfaces which specify
 *   a specific value).
 *
 * Register a preferences interface which will become a part of the preferences
 * widget.
 */
void
swamigui_register_pref_handler(const char *name, const char *icon, int order,
                               SwamiguiPrefHandler handler)
{
    PrefInfo *info;

    g_return_if_fail(name != NULL);
    g_return_if_fail(icon != NULL);
    g_return_if_fail(handler != NULL);

    info = swamigui_pref_info_new();
    info->name = g_strdup(name);
    info->icon = g_strdup(icon);
    info->order = order;
    info->handler = handler;

    pref_list = g_list_insert_sorted(pref_list, info,
                                     sort_pref_info_by_order_and_name);
}

/* GCompareFunc for sorting list items by order value or name */
static gint
sort_pref_info_by_order_and_name(gconstpointer a, gconstpointer b)
{
    PrefInfo *ainfo = (PrefInfo *)a, *binfo = (PrefInfo *)b;

    if(ainfo->order != SWAMIGUI_PREF_ORDER_NAME
            && binfo->order != SWAMIGUI_PREF_ORDER_NAME)
    {
        return (ainfo->order - binfo->order);
    }

    if(ainfo->order != SWAMIGUI_PREF_ORDER_NAME
            && binfo->order == SWAMIGUI_PREF_ORDER_NAME)
    {
        return (-1);
    }

    if(ainfo->order == SWAMIGUI_PREF_ORDER_NAME
            && binfo->order != SWAMIGUI_PREF_ORDER_NAME)
    {
        return (1);
    }

    return (strcmp(ainfo->name, binfo->name));
}

static void
swamigui_pref_class_init(SwamiguiPrefClass *klass)
{
    /* Add built in preference sections */
    swamigui_register_pref_handler(_("General"), GTK_STOCK_PREFERENCES, 10,
                                   general_prefs_handler);
    swamigui_register_pref_handler(_("Audio Samples"), SWAMIGUI_STOCK_SAMPLE_VIEWER, 15,
                                   audio_samples_prefs_handler);
    /* FIXME - Install a custom keyboard icon */
    swamigui_register_pref_handler(_("Keyboard Map"), GTK_STOCK_SELECT_FONT, 20,
                                   keyboard_prefs_handler);
}

static void
swamigui_pref_init(SwamiguiPref *pref)
{
    GtkWidget *prefwidg;
    GtkWidget *treeview;
    GtkWidget *widg;
    GtkWidget *btn;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkTreeSelection *selection;
    GtkListStore *store;
    GtkTreeIter iter;
    PrefInfo *info;
    GList *p;

    gtk_window_set_title(GTK_WINDOW(pref), _("Preferences"));

    /* Create the preferences glade widget and pack it in the dialog */
    prefwidg = swamigui_util_glade_create("Preferences");
    gtk_widget_show(prefwidg);

    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(pref)->vbox), prefwidg, TRUE, TRUE, 0);

    /* Add close button to button box */
    btn = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
    gtk_widget_show(btn);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(pref)->action_area), btn);
    g_signal_connect(btn, "clicked", G_CALLBACK(swamigui_pref_cb_close_clicked), pref);

    treeview = swamigui_util_glade_lookup(prefwidg, "TreeViewSections");
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));

    g_signal_connect(selection, "changed",
                     G_CALLBACK(swamigui_pref_section_list_change), pref);

    /* create the list store for the preference sections list and assign to tree view */
    store = gtk_list_store_new(SECTIONS_COLUMN_COUNT, G_TYPE_STRING,
                               G_TYPE_STRING);
    gtk_tree_view_set_model(GTK_TREE_VIEW(treeview), GTK_TREE_MODEL(store));

    /* add icon and name cell renderer columns to tree */
    renderer = gtk_cell_renderer_pixbuf_new();
    g_object_set(renderer, "stock-size", GTK_ICON_SIZE_BUTTON, NULL);
    column = gtk_tree_view_column_new_with_attributes("icon", renderer,
             "stock-id", SECTIONS_COLUMN_ICON,
             NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("name", renderer,
             "text", SECTIONS_COLUMN_NAME,
             NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

    /* each section is stored in a notebook page.  Notebook tabs are invisible. */
    pref->notebook = swamigui_util_glade_lookup(prefwidg, "NoteBookPanels");

    /* add preferences sections to list and control interfaces to invisible notebook */
    for(p = pref_list; p; p = p->next)
    {
        info = (PrefInfo *)(p->data);

        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter,
                           SECTIONS_COLUMN_ICON, info->icon,
                           SECTIONS_COLUMN_NAME, info->name,
                           -1);

        widg = info->handler();
        gtk_notebook_append_page(GTK_NOTEBOOK(pref->notebook), widg, NULL);
    }

    /* Select the first item in the list */
    if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter))
    {
        gtk_tree_selection_select_iter(GTK_TREE_SELECTION(selection), &iter);
    }
}

static void
swamigui_pref_cb_close_clicked(GtkWidget *button, gpointer user_data)
{
    SwamiguiPref *pref = SWAMIGUI_PREF(user_data);
    gtk_widget_destroy(GTK_WIDGET(pref));	/* destroy dialog */
}

/* callback for when preference section list selection changes */
static void
swamigui_pref_section_list_change(GtkTreeSelection *selection, gpointer user_data)
{
    SwamiguiPref *pref = SWAMIGUI_PREF(user_data);
    GtkTreeModel *model;
    GtkTreeIter iter;
    GtkTreePath *path;
    gint *indices;

    if(!gtk_tree_selection_get_selected(selection, &model, &iter))
    {
        return;
    }

    path = gtk_tree_model_get_path(model, &iter);	/* ++ alloc path */
    indices = gtk_tree_path_get_indices(path);

    /* set the notebook to page corresponding to selected section index */
    if(indices && indices[0] != -1)
    {
        gtk_notebook_set_current_page(GTK_NOTEBOOK(pref->notebook), indices[0]);
    }

    gtk_tree_path_free(path);	/* -- free path */
}

/**
 * swamigui_pref_new:
 *
 * Create preferences dialog widget.
 *
 * Returns: New preferences dialog widget.
 */
GtkWidget *
swamigui_pref_new(void)
{
    return ((GtkWidget *)g_object_new(SWAMIGUI_TYPE_PREF, NULL));
}

/* General preferences widget handler */
static GtkWidget *
general_prefs_handler(void)
{
    GtkWidget *widg;

    widg = swamigui_util_glade_create("GeneralPrefs");
    swamigui_control_glade_prop_connect(widg, G_OBJECT(swamigui_root));
    gtk_widget_show(widg);

    return (widg);
}

/* Audio Samples preferences widget handler */
static GtkWidget *
audio_samples_prefs_handler(void)
{
    GtkWidget *widg;

    widg = swamigui_util_glade_create("SamplePrefs");
    swamigui_control_glade_prop_connect(widg, G_OBJECT(swamigui_root));
    gtk_widget_show(widg);

    return (widg);
}

/* Virtual keyboard mapping preferences handler */
static GtkWidget *
keyboard_prefs_handler(void)
{
    GtkWidget *prefwidg;
    GtkWidget *treeview;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkTreeSelection *selection;
    GtkListStore *store;
    GtkWidget *widg;

    prefwidg = swamigui_util_glade_create("VirtKeyboardPrefs");

    treeview = swamigui_util_glade_lookup(prefwidg, "KeyTreeView");

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
    gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);
    g_object_set_data(G_OBJECT(prefwidg), "selection", selection);
//  g_signal_connect (selection, "changed",
//		    G_CALLBACK (keybindings_selection_changed), prefwidg);

    /* create the list store for the key bindings */
    store = gtk_list_store_new(KEYBIND_COLUMN_COUNT, G_TYPE_STRING, G_TYPE_STRING);
    gtk_tree_view_set_model(GTK_TREE_VIEW(treeview), GTK_TREE_MODEL(store));
    g_object_set_data(G_OBJECT(prefwidg), "store", store);

    /* add note and key name cell renderer columns to tree */
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Note", renderer,
             "text", KEYBIND_COLUMN_NOTE,
             NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Key binding", renderer,
             "text", KEYBIND_COLUMN_KEY,
             NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

    keybindings_update(prefwidg, TRUE);	/* update key binding list to lower keys */

    widg = swamigui_util_glade_lookup(prefwidg, "RadioLower");
    g_signal_connect(widg, "toggled", G_CALLBACK(keybindings_lower_radio_toggled),
                     prefwidg);

    widg = swamigui_util_glade_lookup(prefwidg, "BtnAddKey");
    g_signal_connect(widg, "toggled", G_CALLBACK(keybindings_add_key_button_toggled),
                     prefwidg);

    widg = swamigui_util_glade_lookup(prefwidg, "BtnChangeKey");
    g_signal_connect(widg, "toggled", G_CALLBACK(keybindings_change_key_button_toggled),
                     prefwidg);

    widg = swamigui_util_glade_lookup(prefwidg, "BtnDeleteKey");
    g_signal_connect(widg, "clicked", G_CALLBACK(keybindings_delete_key_button_clicked),
                     prefwidg);

    widg = swamigui_util_glade_lookup(prefwidg, "BtnResetKeys");
    g_signal_connect(widg, "clicked", G_CALLBACK(keybindings_reset_keys_button_clicked),
                     prefwidg);

    gtk_widget_show(prefwidg);

    return (prefwidg);
}

#if 0
/* callback when keybindings list selection changes */
static void
keybindings_selection_changed(GtkTreeSelection *selection, gpointer user_data)
{
    GtkWidget *prefwidg = GTK_WIDGET(user_data);
    GtkWidget *widg;

#if 0
    widg = swamigui_util_glade_lookup(prefwidg, "LabelKeyBind");

    /* If key binding is active, de-activate it */
    if(GTK_WIDGET_VISIBLE(widg))
    {
        gtk_widget_hide(widg);

        g_signal_handlers_disconnect_by_func(prefwidg,
                                             G_CALLBACK(keybindings_key_press_event),
                                             prefwidg);
    }

#endif

    /* Set change button sensitivity depending on if only one list item selected */
    widg = swamigui_util_glade_lookup(prefwidg, "BtnChangeKey");
    gtk_widget_set_sensitive(widg, gtk_tree_selection_count_selected_rows(selection) == 1);
}
#endif

/* Update keybindings list to lower (lower == TRUE) or upper (lower == FALSE) */
static void
keybindings_update(GtkWidget *prefwidg, gboolean lower)
{
    char *propval;
    char **keys, **sptr;
    char notename[16];
    GtkTreeIter iter;
    GtkListStore *store;
    int i;

    store = g_object_get_data(G_OBJECT(prefwidg), "store");

    gtk_list_store_clear(store);

    g_object_get(swamigui_root, lower ? "piano-lower-keys" : "piano-upper-keys",
                 &propval, NULL);	/* ++ alloc string */

    keys = g_strsplit(propval, ",", 0);	/* ++ alloc (NULL terminated array of strings) */
    g_free(propval);	/* -- free string */

    /* add preferences sections to list and control interfaces to invisible notebook */
    for(sptr = keys, i = 0; *sptr; sptr++, i++)
    {
        gtk_list_store_append(store, &iter);

        sprintf(notename, "%s%d", note_names[i % 12], i / 12);

        gtk_list_store_set(store, &iter,
                           KEYBIND_COLUMN_NOTE, notename,
                           KEYBIND_COLUMN_KEY, *sptr,
                           -1);
    }

    g_strfreev(keys);	/* -- free array of strings */
}

static void
keybindings_set_bind_mode(GtkWidget *prefwidg, BindMode mode)
{
    gboolean addbtn = FALSE, changebtn = FALSE;
    GtkWidget *widg;
    GtkWidget *lbl;

    /* Check if requested mode is already set */
    if(GPOINTER_TO_INT(g_object_get_data(G_OBJECT(prefwidg), "bind-mode")) == mode)
    {
        return;
    }

    lbl = swamigui_util_glade_lookup(prefwidg, "LabelKeyBind");

    switch(mode)
    {
    case BIND_MODE_ADD:
        addbtn = TRUE;

    /* fall through */
    case BIND_MODE_CHANGE:
        if(mode == BIND_MODE_CHANGE)
        {
            changebtn = TRUE;
        }

        gtk_widget_show(lbl);
        g_signal_connect(prefwidg, "key-press-event",
                         G_CALLBACK(keybindings_key_press_event), prefwidg);
        break;

    case BIND_MODE_INACTIVE:
        gtk_widget_hide(lbl);
        g_signal_handlers_disconnect_by_func(prefwidg,
                                             G_CALLBACK(keybindings_key_press_event),
                                             prefwidg);
        break;
    }

    widg = swamigui_util_glade_lookup(prefwidg, "BtnAddKey");
    g_signal_handlers_block_by_func(widg, keybindings_add_key_button_toggled,
                                    prefwidg);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widg), addbtn);
    g_signal_handlers_unblock_by_func(widg, keybindings_add_key_button_toggled,
                                      prefwidg);

    widg = swamigui_util_glade_lookup(prefwidg, "BtnChangeKey");
    g_signal_handlers_block_by_func(widg, keybindings_change_key_button_toggled,
                                    prefwidg);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widg), changebtn);
    g_signal_handlers_unblock_by_func(widg, keybindings_change_key_button_toggled,
                                      prefwidg);

    /* indicate the current key bind mode */
    g_object_set_data(G_OBJECT(prefwidg), "bind-mode", GUINT_TO_POINTER(mode));
}

/* callback when Lower radio button is toggled */
static void
keybindings_lower_radio_toggled(GtkToggleButton *btn, gpointer user_data)
{
    GtkWidget *prefwidg = GTK_WIDGET(user_data);

    keybindings_update(prefwidg, gtk_toggle_button_get_active(btn));
}

static void
keybindings_add_key_button_toggled(GtkToggleButton *btn, gpointer user_data)
{
    GtkWidget *prefwidg = GTK_WIDGET(user_data);
    keybindings_set_bind_mode(prefwidg, gtk_toggle_button_get_active(btn)
                              ? BIND_MODE_ADD : BIND_MODE_INACTIVE);
}

static void
keybindings_change_key_button_toggled(GtkToggleButton *btn, gpointer user_data)
{
    GtkWidget *prefwidg = GTK_WIDGET(user_data);
    keybindings_set_bind_mode(prefwidg, gtk_toggle_button_get_active(btn)
                              ? BIND_MODE_CHANGE : BIND_MODE_INACTIVE);
}

static void
keybindings_delete_key_button_clicked(GtkButton *btn, gpointer user_data)
{
    GtkWidget *prefwidg = GTK_WIDGET(user_data);
    GtkTreeSelection *selection;
    GtkListStore *store;
    GList *sel_iters = NULL, *p;
    GtkTreeIter *iter;

    /* de-activate bind mode, if any */
    keybindings_set_bind_mode(prefwidg, BIND_MODE_INACTIVE);

    selection = g_object_get_data(G_OBJECT(prefwidg), "selection");
    store = g_object_get_data(G_OBJECT(prefwidg), "store");

    gtk_tree_selection_selected_foreach(selection, keybindings_selection_foreach,
                                        &sel_iters);	/* ++ alloc */

    for(p = sel_iters; p; p = p->next)
    {
        iter = (GtkTreeIter *)(p->data);

        gtk_list_store_remove(store, iter);
        gtk_tree_iter_free(iter);
    }

    g_list_free(sel_iters);	/* -- free */
}

/* Callback for when Reset keys button is clicked */
static void
keybindings_reset_keys_button_clicked(GtkButton *btn, gpointer user_data)
{
    GtkWidget *prefwidg = GTK_WIDGET(user_data);
    GtkWidget *dialog;

    /* de-activate bind mode, if any */
    keybindings_set_bind_mode(prefwidg, BIND_MODE_INACTIVE);

    dialog =
        gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT,
                               GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
                               _("Reset all piano key bindings to defaults?"));
    g_signal_connect(GTK_DIALOG(dialog), "response",
                     G_CALLBACK(keybindings_reset_keys_response), prefwidg);
    gtk_widget_show(dialog);
}

/* Callback for response to "Reset all keys?" dialog */
static void
keybindings_reset_keys_response(GtkDialog *dialog, int response, gpointer user_data)
{
    GtkWidget *prefwidg = GTK_WIDGET(user_data);
    GParamSpec *pspec;
    GValue value = { 0 };
    GtkWidget *widg;

    if(response == GTK_RESPONSE_YES)	/* Reset to defaults if Yes */
    {
        pspec = g_object_class_find_property(g_type_class_peek(SWAMIGUI_TYPE_ROOT),
                                             "piano-lower-keys");
        g_value_init(&value, G_TYPE_STRING);
        g_param_value_set_default(pspec, &value);
        g_object_set_property(G_OBJECT(swamigui_root), "piano-lower-keys", &value);

        g_value_reset(&value);

        pspec = g_object_class_find_property(g_type_class_peek(SWAMIGUI_TYPE_ROOT),
                                             "piano-upper-keys");
        g_param_value_set_default(pspec, &value);
        g_object_set_property(G_OBJECT(swamigui_root), "piano-upper-keys", &value);

        /* Update keybindings list */
        widg = swamigui_util_glade_lookup(prefwidg, "RadioLower");
        keybindings_update(prefwidg,
                           gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widg)));
    }

    gtk_object_destroy(GTK_OBJECT(dialog));
}

/* a tree selection foreach callback to remove selected modulators */
static void
keybindings_selection_foreach(GtkTreeModel *model, GtkTreePath *path,
                              GtkTreeIter *iter, gpointer data)
{
    GList **sel_iters = (GList **)data;
    GtkTreeIter *copy;

    copy = gtk_tree_iter_copy(iter);
    *sel_iters = g_list_append(*sel_iters, copy);
}

/* callback when a key is pressed (for key binding) */
static gboolean
keybindings_key_press_event(GtkWidget *eventwidg, GdkEventKey *event, gpointer user_data)
{
    GtkWidget *prefwidg = GTK_WIDGET(user_data);
    GtkTreeSelection *selection;
    GtkWidget *treeview;
    GtkListStore *store;
    GtkTreeIter iter;
    GtkTreePath *path;
    GList *list;
    guint mode;
    char *keyname;
    char notename[16];
    int count;

    if(event->keyval == GDK_Escape)	/* If escape, stop key binding mode */
    {
        keybindings_set_bind_mode(prefwidg, BIND_MODE_INACTIVE);
        return (TRUE);	/* Stop key propagation */
    }

    store = g_object_get_data(G_OBJECT(prefwidg), "store");
    selection = g_object_get_data(G_OBJECT(prefwidg), "selection");
    treeview = swamigui_util_glade_lookup(prefwidg, "KeyTreeView");
    mode = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(prefwidg), "bind-mode"));

    switch(mode)
    {
    case BIND_MODE_ADD:	/* Add new keybinding to list */
        keyname = gdk_keyval_name(event->keyval);
        count = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(store), NULL);

        gtk_list_store_append(store, &iter);

        sprintf(notename, "%s%d", note_names[count % 12], count / 12);

        gtk_list_store_set(store, &iter,
                           KEYBIND_COLUMN_NOTE, notename,
                           KEYBIND_COLUMN_KEY, keyname,
                           -1);

        /* select new item and scroll to it */
        gtk_tree_selection_unselect_all(selection);
        gtk_tree_selection_select_iter(selection, &iter);

        path = gtk_tree_model_get_path(GTK_TREE_MODEL(store), &iter);	/* ++ alloc */
        gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(treeview), path, NULL,
                                     FALSE, 0.0, 0.0);
        gtk_tree_path_free(path);	/* -- free */
        break;

    case BIND_MODE_CHANGE:
        keyname = gdk_keyval_name(event->keyval);

        list = gtk_tree_selection_get_selected_rows(selection, NULL);	/* ++ alloc list */

        /* if there is a selected item.. */
        if(list && gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter,
                                           (GtkTreePath *)(list->data)))
        {
            count = gtk_tree_path_get_indices((GtkTreePath *)(list->data))[0];
            sprintf(notename, "%s%d", note_names[count % 12], count / 12);

            gtk_list_store_set(store, &iter,
                               KEYBIND_COLUMN_NOTE, notename,
                               KEYBIND_COLUMN_KEY, keyname,
                               -1);

            /* Select the next item in the list */
            if(gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter))
            {
                gtk_tree_selection_unselect_all(selection);
                gtk_tree_selection_select_iter(selection, &iter);

                path = gtk_tree_model_get_path(GTK_TREE_MODEL(store), &iter);	/* ++ alloc */
                gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(treeview), path, NULL,
                                             FALSE, 0.0, 0.0);
                gtk_tree_path_free(path);	/* -- free */
            }	/* No more items, de-activate change mode */
            else
            {
                keybindings_set_bind_mode(prefwidg, BIND_MODE_INACTIVE);
            }
        }

        /* -- free list */
        g_list_foreach(list, (GFunc)gtk_tree_path_free, NULL);
        g_list_free(list);
        break;
    }

    /* Sync updated keys to SwamiguiRoot property */
    keybindings_sync(prefwidg);

    return (TRUE);	/* Stop key propagation */
}

/* Synchronize key list to SwamiguiRoot property */
static void
keybindings_sync(GtkWidget *prefwidg)
{
    GtkTreeModel *model;
    GtkTreeIter iter;
    GString *string;
    gboolean lowkeys;
    GtkWidget *radio;
    char *keyname;

    model = GTK_TREE_MODEL(g_object_get_data(G_OBJECT(prefwidg), "store"));

    string = g_string_new("");	/* ++ alloc */

    if(gtk_tree_model_get_iter_first(model, &iter))
    {
        do
        {
            gtk_tree_model_get(model, &iter,
                               KEYBIND_COLUMN_KEY, &keyname,		/* ++ alloc */
                               -1);

            if(string->len != 0)
            {
                g_string_append_c(string, ',');
            }

            g_string_append(string, keyname);
            g_free(keyname);		/* -- free key name */
        }
        while(gtk_tree_model_iter_next(model, &iter));
    }

    radio = swamigui_util_glade_lookup(prefwidg, "RadioLower");
    lowkeys = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radio));

    g_object_set(swamigui_root, lowkeys ? "piano-lower-keys" : "piano-upper-keys",
                 string->str, NULL);
    g_string_free(string, TRUE);		/* -- free */
}
