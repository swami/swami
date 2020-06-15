/*
 * SwamiguiMultiSave.h - Multiple file save dialog
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
#include <stdlib.h>
#include <stdarg.h>
#include <gtk/gtk.h>
#include <glib/gstdio.h>
#include <libinstpatch/libinstpatch.h>

#include <libswami/SwamiRoot.h>

#include "SwamiguiRoot.h"
#include "SwamiguiMultiSave.h"

#include "i18n.h"

enum
{
    SAVE_COLUMN,		/* save check box */
    CHANGED_COLUMN,	/* changed text */
    TITLE_COLUMN,		/* title of item */
    PATH_COLUMN,		/* file path */
    ITEM_COLUMN,		/* associated IpatchItem */
    N_COLUMNS
};

static void swamigui_multi_save_finalize(GObject *object);
static GtkFileChooserConfirmation
warning_overwrite_callback(GtkFileChooser *chooser, gpointer data);
static void save_toggled(GtkCellRendererToggle *cell, char *path_str,
                         gpointer data);
static void save_column_clicked(GtkTreeViewColumn *column, gpointer data);
static gboolean
swamigui_multi_save_treeview_query_tooltip(GtkWidget *widget,
        gint x, gint y, gboolean keyboard_mode,
        GtkTooltip *tooltip, gpointer user_data);
static void multi_save_response(GtkDialog *dialog, int response,
                                gpointer user_data);

G_DEFINE_TYPE(SwamiguiMultiSave, swamigui_multi_save, GTK_TYPE_DIALOG);


static void
swamigui_multi_save_class_init(SwamiguiMultiSaveClass *klass)
{
    GObjectClass *obj_class = G_OBJECT_CLASS(klass);

    obj_class->finalize = swamigui_multi_save_finalize;
}

static void
swamigui_multi_save_init(SwamiguiMultiSave *multi)
{
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkWidget *hbox;
    GtkWidget *frame;
    GtkWidget *btn;
    GtkWidget *image;
    GtkTooltips *tooltips;

    /* tool tips for dialog widgets */
    tooltips = gtk_tooltips_new();

    gtk_window_set_default_size(GTK_WINDOW(multi), 600, 300);

    hbox = gtk_hbox_new(FALSE, 8);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(multi)->vbox), hbox, FALSE, FALSE, 8);

    /* icon image */
    multi->icon = gtk_image_new_from_stock(GTK_STOCK_SAVE, GTK_ICON_SIZE_DIALOG);
    gtk_box_pack_start(GTK_BOX(hbox), multi->icon, FALSE, FALSE, 0);

    /* message label */
    multi->message = gtk_label_new("");
    gtk_label_set_line_wrap(GTK_LABEL(multi->message), TRUE);
    gtk_box_pack_start(GTK_BOX(hbox), multi->message, FALSE, FALSE, 0);

    /* browse button */
    btn = gtk_button_new_with_label (_("\"Save file as\" browser"));
    image = gtk_image_new_from_stock(GTK_STOCK_OPEN, GTK_ICON_SIZE_BUTTON);
    gtk_button_set_image(GTK_BUTTON(btn), image);
    gtk_box_pack_end(GTK_BOX(hbox), btn, FALSE, FALSE, 0);
    g_signal_connect(btn, "clicked", G_CALLBACK (swamigui_save_as_browser), multi);

    gtk_widget_show_all(hbox);

    /* frame for list */
    frame = gtk_frame_new(NULL);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(multi)->vbox), frame, TRUE, TRUE, 0);

    /* scroll window for list */
    multi->scroll_win = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(multi->scroll_win),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_show(multi->scroll_win);
    gtk_container_add(GTK_CONTAINER(frame), multi->scroll_win);

    /* ++ ref list store */
    multi->store = gtk_list_store_new(N_COLUMNS, G_TYPE_BOOLEAN, G_TYPE_STRING,
                                      G_TYPE_STRING, G_TYPE_STRING,
                                      IPATCH_TYPE_ITEM);
    /* tree view */
    multi->treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(multi->store));
    gtk_widget_set_has_tooltip(multi->treeview, TRUE);
    g_signal_connect(multi->treeview, "query-tooltip",
                     G_CALLBACK(swamigui_multi_save_treeview_query_tooltip), multi);
    gtk_container_add(GTK_CONTAINER(multi->scroll_win), multi->treeview);

    renderer = gtk_cell_renderer_toggle_new();
    g_signal_connect(renderer, "toggled", G_CALLBACK(save_toggled), multi->store);
    column = gtk_tree_view_column_new_with_attributes(_("Save"), renderer,
             "active", SAVE_COLUMN,
             NULL);
    gtk_tree_view_column_set_clickable(GTK_TREE_VIEW_COLUMN(column), TRUE);
    g_signal_connect(column, "clicked", G_CALLBACK(save_column_clicked), multi);
    gtk_tree_view_append_column(GTK_TREE_VIEW(multi->treeview), column);
    gtk_tooltips_set_tip(tooltips, GTK_TREE_VIEW_COLUMN(column)->button,
                         _("Select which files to save."), NULL);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(_("Changed"), renderer,
             "text", CHANGED_COLUMN,
             NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(multi->treeview), column);
    gtk_tooltips_set_tip(tooltips, GTK_TREE_VIEW_COLUMN(column)->button,
                         _("File changed since last save?"), NULL);

    renderer = gtk_cell_renderer_text_new();
    g_object_set(renderer,
                 "ellipsize", PANGO_ELLIPSIZE_END,
                 "ellipsize-set", TRUE,
                 NULL);
    column = gtk_tree_view_column_new_with_attributes("Title", renderer,
             "text", TITLE_COLUMN,
             NULL);
    g_object_set(column,
                 "resizable", TRUE,
                 "expand", TRUE,
                 NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(multi->treeview), column);

    /* create column with path text: not editable */
    renderer = gtk_cell_renderer_text_new();
    g_object_set(renderer,
                 "ellipsize", PANGO_ELLIPSIZE_START,
                 "ellipsize-set", TRUE,
                 NULL);

    column = gtk_tree_view_column_new_with_attributes("Path", renderer,
             "text", PATH_COLUMN,
             NULL);
    g_object_set(column,
                 "resizable", TRUE,
                 "expand", TRUE,
                 NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(multi->treeview), column);

    gtk_widget_show_all(frame);

    gtk_dialog_add_buttons(GTK_DIALOG(multi),
                           GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                           NULL);

    multi->accept_btn = gtk_dialog_add_button(GTK_DIALOG(multi), GTK_STOCK_SAVE,
                        GTK_RESPONSE_ACCEPT);

    g_signal_connect(multi, "response", G_CALLBACK(multi_save_response), NULL);
}

static void
swamigui_multi_save_finalize(GObject *object)
{
    SwamiguiMultiSave *multi = SWAMIGUI_MULTI_SAVE(object);

    g_object_unref(multi->store);         // -- unref list store

    if(G_OBJECT_CLASS(swamigui_multi_save_parent_class)->finalize)
    {
        G_OBJECT_CLASS(swamigui_multi_save_parent_class)->finalize(object);
    }
}

/* browse button clicked callback */
void swamigui_save_as_browser(GtkButton *button, gpointer user_data)
{
    SwamiguiMultiSave *multi = SWAMIGUI_MULTI_SAVE(user_data);
    GtkWidget *filesel;
    GtkTreeModel *model;
    GtkTreeSelection *selection;
    GtkTreeIter iter;
    char *fname;

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(multi->treeview));

    if(!gtk_tree_selection_get_selected(selection, &model, &iter))
    {
        return;
    }

    gtk_tree_model_get(model, &iter,
                       PATH_COLUMN, &fname,	/* ++ alloc */
                       -1);

    filesel =
        gtk_file_chooser_dialog_new(_("Save file as"),
                                    GTK_WINDOW(multi), GTK_FILE_CHOOSER_ACTION_SAVE,
                                    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                    GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
                                    NULL);

    /* Enable GtkFileChooserDialog to verify when the user choose a file that
       already exits */
    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER (filesel), TRUE);

    /* Connect a custom callback when the user choose a file that already exists. */
    /* The callback will receive the initial file fname in parameter. */
    g_signal_connect(filesel, "confirm-overwrite",
                     G_CALLBACK (warning_overwrite_callback), fname);

    if(!fname)
    {
        g_object_get(swami_root, "patch-path", &fname, NULL);     // ++ alloc patch path

        if(fname)
        {
            gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(filesel), fname);
        }
    }
    else
    {
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(filesel), fname);
    }

    /* make the dialog modal by waiting for the user response */
    if(gtk_dialog_run(GTK_DIALOG (filesel)) == GTK_RESPONSE_ACCEPT)
    {
        char *new_fname;
        IpatchItem *item;
        gboolean changed, saved;
        GError *err = NULL;

        /* get the new filename chosen by the user, save the file and put
           new file name in multisave dialog */
        /* ++ alloc file name from file chooser */
        new_fname = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(filesel));

        /* Get the item to save */
        gtk_tree_model_get(model, &iter,
                           ITEM_COLUMN, &item,     /* ++ ref item */
                           -1);

        /* save the item under new file name */
        if(!swami_root_patch_save(item, new_fname, &err))
        {
            /* bad luck, an error occured */
            GtkWidget *msgdialog;
            msgdialog = gtk_message_dialog_new(GTK_WINDOW (filesel), 0,
                                               GTK_MESSAGE_ERROR,
                                               GTK_BUTTONS_OK,
                                               _("Error saving '%s': %s"),
                                               new_fname, ipatch_gerror_message(err));
            g_clear_error(&err);

            /* Waiting for user response */
            gtk_dialog_run(GTK_DIALOG(msgdialog));
            gtk_widget_destroy(msgdialog);
        }
        else
        {
            /* file has been saved, update row in multi save GtkListStore */
            /* Get new item states: "changed" , "saved" */
            g_object_get(item,
                         "changed", &changed,  /* changed state */
                         "saved", &saved,      /* saved state */
                         NULL);

            /* set saved, changed and new filename in respective column */
            gtk_list_store_set(multi->store, &iter,
                               SAVE_COLUMN, saved,
                               CHANGED_COLUMN, changed ? _("Yes") : _("No"),
                               PATH_COLUMN, new_fname,
                               -1);
        }
        g_free(new_fname);         /* free file name string */
        g_object_unref(item);  /* -- unref item */
    }

    g_free(fname);		/* -- free file name string */

    /* close "save file as" dialog */
    gtk_widget_destroy(filesel);
}

/**
 * The new file name choosed by the user is a file that already exists.
 * The function checks if this file is allowed to be overwritten.
 * The rule of precedence is (1) else (2) else (3):
 *  (1) new file name identical to initial file name is allowed.
 *
 *  (2) new file name corresponding to a file in use in swami is
 *      not allowed to be overwritten.
 *      The user is requested to choose another file name.
 *
 *  (3) new file name is not in use by swami, the user is requested to
 *      confirm that the file could be overwritten.
 *
 * Note: Swami consider file name "case insensitive" regardless of the host OS.
 * That means that f.sf2 should be considered the same file as F.sf2.
 */
static GtkFileChooserConfirmation
warning_overwrite_callback(GtkFileChooser *chooser, gpointer data)
{
    char *init_fname = (char*)data; /* initial filename */

    /* return value */
    GtkFileChooserConfirmation ret = GTK_FILE_CHOOSER_CONFIRMATION_SELECT_AGAIN;
    GtkWidget *msgdialog;
    GtkButtonsType gtk_button_type;
    const char *message;
    char *new_fname;
    gint resp;

    /* get new file name from "save as" dialog */
    new_fname = gtk_file_chooser_get_filename(chooser); /* ++ alloc */

    /* check if new_fname is identical to initial file name */
    /* note that the comparison is "case insensitive" */
    if(init_fname && g_ascii_strcasecmp(init_fname, new_fname) == 0)
    {
        g_free(new_fname);
        return GTK_FILE_CHOOSER_CONFIRMATION_ACCEPT_FILENAME;
    }

    /* check if new_fname corresponds to a file already loaded */
    if( swami_root_patch_is_loaded(SWAMI_ROOT(swamigui_root), new_fname))
    {
        const static char *not_allowed_msg =
        "Overwritten file in use '%s' is not allowed.\nPlease choose a new name.";

        message = not_allowed_msg;
        gtk_button_type = GTK_BUTTONS_OK;
    }
    else
    {
        /* the user is requested to confirm that the file should be overwritten.*/
        const static char *overwriting_msg =
        "File %s already exists. Do you want to overwrite this file ?";

        message = overwriting_msg;
        gtk_button_type = GTK_BUTTONS_YES_NO;
    }

    /* warn the user about overwriting existing file */
    msgdialog = gtk_message_dialog_new(GTK_WINDOW (chooser), 0,
                                       GTK_MESSAGE_WARNING,
                                       gtk_button_type,
                                       message,
                                       new_fname);

    /* Waiting for user response */
    resp = gtk_dialog_run(GTK_DIALOG(msgdialog));
    if((gtk_button_type == GTK_BUTTONS_YES_NO) && (resp == GTK_RESPONSE_YES))
    {
        g_unlink(new_fname);  /* delete existing file before saving */
        ret = GTK_FILE_CHOOSER_CONFIRMATION_ACCEPT_FILENAME;
    }

    gtk_widget_destroy(msgdialog);

    /* free file name string */
    g_free(new_fname); /* free */
    return ret;
}

static void
save_toggled(GtkCellRendererToggle *cell, char *path_str, gpointer data)
{
    GtkTreeModel *model = (GtkTreeModel *)data;
    GtkTreeIter iter;
    GtkTreePath *path;
    gboolean save;

    /* ++ alloc path from string */
    path = gtk_tree_path_new_from_string(path_str);

    /* get toggled iter */
    gtk_tree_model_get_iter(model, &iter, path);
    gtk_tree_model_get(model, &iter, SAVE_COLUMN, &save, -1);

    save ^= 1;	/* toggle the value */

    /* set new value */
    gtk_list_store_set(GTK_LIST_STORE(model), &iter, SAVE_COLUMN, save, -1);

    gtk_tree_path_free(path);	/* -- free path */
}

// Callback when "save" column button gets clicked (toggle all save checkboxes)
static void
save_column_clicked(GtkTreeViewColumn *column, gpointer data)
{
    GtkTreeView *treeview = GTK_TREE_VIEW(gtk_tree_view_column_get_tree_view(column));
    GtkTreeModel *model = gtk_tree_view_get_model(treeview);
    gboolean all_checked = TRUE;
    GtkTreeIter iter;
    gboolean save;

    if(!gtk_tree_model_get_iter_first(model, &iter))
    {
        return;
    }

    do
    {
        gtk_tree_model_get(model, &iter,
                           SAVE_COLUMN, &save,
                           -1);

        if(!save)
        {
            all_checked = FALSE;
            break;
        }
    }
    while(gtk_tree_model_iter_next(model, &iter));

    gtk_tree_model_get_iter_first(model, &iter);

    do
    {
        gtk_list_store_set(GTK_LIST_STORE(model), &iter,
                           SAVE_COLUMN, !all_checked,
                           -1);
    }
    while(gtk_tree_model_iter_next(model, &iter));
}

// Signal handler for query-tooltip on tree view
static gboolean
swamigui_multi_save_treeview_query_tooltip(GtkWidget *widget,
        gint x, gint y, gboolean keyboard_mode,
        GtkTooltip *tooltip, gpointer user_data)
{
    GtkTreeViewColumn *column;
    GtkTreeModel *model;
    GtkTreePath *path;
    GtkTreeIter iter;
    GList *list;
    int colindex;
    char *s;

    if(!gtk_tree_view_get_tooltip_context(GTK_TREE_VIEW(widget), &x, &y,
                                          keyboard_mode, &model, &path, &iter))
    {
        return (FALSE);
    }

    if(!gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(widget), x, y, NULL, &column, NULL, NULL))
    {
        return (FALSE);
    }

    list = gtk_tree_view_get_columns(GTK_TREE_VIEW(widget));      // ++ alloc list
    colindex = g_list_index(list, column);
    g_list_free(list);                                            // -- free list

    if(colindex == 2)
        gtk_tree_model_get(model, &iter,
                           TITLE_COLUMN, &s,       // ++ alloc
                           -1);
    else if(colindex == 3)
        gtk_tree_model_get(model, &iter,
                           PATH_COLUMN, &s,        // ++ alloc
                           -1);
    else
    {
        return (FALSE);
    }

    gtk_tooltip_set_text(tooltip, s);
    g_free(s);            // -- free

    gtk_tree_view_set_tooltip_cell(GTK_TREE_VIEW(widget), tooltip, path, column, NULL);

    return (TRUE);
}

/* called when dialog response received (button clicked by user) */
static void
multi_save_response(GtkDialog *dialog, int response, gpointer user_data)
{
    SwamiguiMultiSave *multi = SWAMIGUI_MULTI_SAVE(dialog);
    GtkTreeModel *model = GTK_TREE_MODEL(multi->store);
    GtkWidget *msgdialog;
    GtkTreeIter iter;
    gboolean save;
    char *path;
    IpatchItem *item;
    GError *err = NULL;
    int result;
    gboolean close_ok;
    IpatchList *close_list;

    if(response != GTK_RESPONSE_ACCEPT)
    {
        gtk_object_destroy(GTK_OBJECT(dialog));
        return;
    }

    /* get first item in list (destroy dialog if no items) */
    if(!gtk_tree_model_get_iter_first(model, &iter))
    {
        gtk_object_destroy(GTK_OBJECT(dialog));
        return;
    }

    close_list = ipatch_list_new();               // ++ ref new list

    do
    {
        gtk_tree_model_get(model, &iter,
                           SAVE_COLUMN, &save,
                           PATH_COLUMN, &path,     /* ++ alloc path */
                           ITEM_COLUMN, &item,     /* ++ ref item */
                           -1);
        close_ok = TRUE;

        if(save)
        {
            if(!swami_root_patch_save(item, path, &err))
            {
                close_ok = FALSE;       /* Don't close file if error on save */

                msgdialog = gtk_message_dialog_new(GTK_WINDOW(dialog), 0,
                                                   GTK_MESSAGE_ERROR,
                                                   GTK_BUTTONS_OK_CANCEL,
                                                   _("Error saving '%s': %s"),
                                                   path, ipatch_gerror_message(err));
                g_clear_error(&err);

                result = gtk_dialog_run(GTK_DIALOG(msgdialog));
                gtk_widget_destroy(msgdialog);

                if(result == GTK_RESPONSE_CANCEL)
                {
                    g_free(path);                 /* -- free path */
                    g_object_unref(item);         /* -- unref item */
                    g_object_unref(close_list);   // -- unref close list
                    return;
                }
            }
        }

        /* Close if in close mode */
        if(close_ok && (multi->flags & SWAMIGUI_MULTI_SAVE_CLOSE_MODE))
        {
            g_object_ref(item);               // ++ ref object for list
            close_list->items = g_list_prepend(close_list->items, item);
        }

        g_free(path);               /* -- free path */
        g_object_unref(item);       /* -- unref item */
    }
    while(gtk_tree_model_iter_next(model, &iter));

    if(close_list->items)
    {
        close_list->items = g_list_reverse(close_list->items);

        if(!ipatch_close_base_list(close_list, &err))
        {
            msgdialog = gtk_message_dialog_new(GTK_WINDOW(dialog), 0,
                                               GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                               _("Error closing files: %s"),
                                               ipatch_gerror_message(err));
            g_clear_error(&err);

            gtk_dialog_run(GTK_DIALOG(msgdialog));
            gtk_widget_destroy(msgdialog);
        }
    }

    g_object_unref(close_list);           // -- unref close list

    gtk_object_destroy(GTK_OBJECT(dialog));
}

/**
 * swamigui_multi_save_new:
 * @title: Title of dialog
 * @message: Message text
 * @flags: A value from #SwamiguiMultiSaveFlags or 0
 *
 * Create a new multi file save dialog.
 *
 * Returns: New dialog widget.
 */
GtkWidget *
swamigui_multi_save_new(char *title, char *message, guint flags)
{
    SwamiguiMultiSave *multi;

    multi = g_object_new(SWAMIGUI_TYPE_MULTI_SAVE, NULL);

    if(title)
    {
        gtk_window_set_title(GTK_WINDOW(multi), title);
    }

    if(message)
    {
        gtk_label_set_text(GTK_LABEL(multi->message), message);
    }

    multi->flags = flags;

    if(flags & SWAMIGUI_MULTI_SAVE_CLOSE_MODE)
    {
        gtk_button_set_label(GTK_BUTTON(multi->accept_btn), GTK_STOCK_CLOSE);
    }

    /* here we are in "Save files" dialog. To retain user attention and disable
       access to menu in other window, the dialog must be modal */
    gtk_window_set_modal(GTK_WINDOW(multi), TRUE);

    /* the dialog is centered on the main window */
    gtk_window_set_transient_for(GTK_WINDOW(multi), GTK_WINDOW(swamigui_root->main_window));

    return (GTK_WIDGET(multi));
}

/**
 * swamigui_multi_save_set_selection:
 * @multi: Multi save widget
 * @selection: Selection of items to save (only #IpatchBase items or children
 *   thereof are considered, children are followed up to their parent
 *   #IpatchBase, duplicates are taken into account).
 *
 * Set the item selection of a multi save dialog.  This is the list of items
 * that the user is prompted to save.
 */
void
swamigui_multi_save_set_selection(SwamiguiMultiSave *multi, IpatchList *selection)
{
    GHashTable *base_hash;
    IpatchItem *item, *base;
    GtkTreeSelection *treesel;
    GtkTreeIter iter;
    char *title, *path;
    gboolean changed, saved, close_mode;
    GList *p;

    g_return_if_fail(SWAMIGUI_IS_MULTI_SAVE(multi));
    g_return_if_fail(IPATCH_IS_LIST(selection));

    close_mode = (multi->flags & SWAMIGUI_MULTI_SAVE_CLOSE_MODE) != 0;

    gtk_list_store_clear(multi->store);

    /* hash to throw out duplicate base objects quickly */
    base_hash = g_hash_table_new(NULL, NULL);

    for(p = selection->items; p; p = p->next)
    {
        item = (IpatchItem *)(p->data);
        base = ipatch_item_get_base(item);  /* ++ ref base */

        if(!base)
        {
            continue;
        }

        /* skip if this base object is already added to the list */
        if(g_hash_table_lookup(base_hash, base))
        {
            g_object_unref(base);             /* -- unref base */
            continue;
        }

        g_hash_table_insert(base_hash, base, GUINT_TO_POINTER(TRUE));

        gtk_list_store_append(multi->store, &iter);

        /* ++ alloc title and path */
        g_object_get(base,
                     "title", &title,      /* ++ alloc title */
                     "file-name", &path,   /* ++ alloc path */
                     "changed", &changed,
                     "saved", &saved,
                     NULL);

        /* SAVE_COLUMN is set to TRUE if save mode or file has already been saved once */
        gtk_list_store_set(multi->store, &iter,
                           SAVE_COLUMN, !close_mode || saved,
                           CHANGED_COLUMN, changed ? _("Yes") : _("No"),
                           TITLE_COLUMN, title,
                           PATH_COLUMN, path,
                           ITEM_COLUMN, base,
                           -1);
        g_free(title);              /* -- free title */
        g_free(path);               /* -- free path */
        g_object_unref(base);       /* -- unref base */
    }

    // Select first item in list
    if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(multi->store), &iter))
    {
        treesel = gtk_tree_view_get_selection(GTK_TREE_VIEW(multi->treeview));
        gtk_tree_selection_select_iter(treesel, &iter);
    }

    g_hash_table_destroy(base_hash);
}
