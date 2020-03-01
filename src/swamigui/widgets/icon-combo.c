/*
 * Ripped and modified for Swami from libgal-0.19.2
 *
 * widget-pixmap-combo.c - A pixmap selector combo box
 * Copyright 2000, 2001, Ximian, Inc.
 *
 * Authors:
 *   Jody Goldberg <jgoldberg@home.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License, version 2, as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include <gtk/gtkbutton.h>
#include <gtk/gtksignal.h>
#include <gtk/gtktable.h>
#include <gtk/gtkimage.h>
#include "combo-box.h"
#include "icon-combo.h"

/* from Swami src/swamigui */
#include "i18n.h"

#define ICON_PREVIEW_WIDTH 15
#define ICON_PREVIEW_HEIGHT 15

enum
{
    CHANGED,
    LAST_SIGNAL
};

static void icon_combo_select_icon_index(IconCombo *ic, int index);

static guint icon_combo_signals[LAST_SIGNAL] = { 0, };
static GObjectClass *icon_combo_parent_class;

/***************************************************************************/

static void
icon_combo_finalize(GObject *object)
{
    IconCombo *ic = ICON_COMBO(object);

    //  g_object_unref (GTK_OBJECT (ic->tool_tip));

    g_free(ic->icons);
    (*icon_combo_parent_class->finalize)(object);
}

static void
icon_combo_class_init(IconComboClass *klass)
{
    GObjectClass *obj_class = G_OBJECT_CLASS(klass);

    obj_class->finalize = icon_combo_finalize;

    icon_combo_parent_class = g_type_class_peek_parent(klass);

    icon_combo_signals[CHANGED] =
        g_signal_new("changed", G_TYPE_FROM_CLASS(klass), G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(IconComboClass, changed), NULL, NULL,
                     g_cclosure_marshal_VOID__INT, GTK_TYPE_NONE,
                     1, G_TYPE_INT);
}

GType
icon_combo_get_type(void)
{
    static GType type = 0;

    if(!type)
    {
        GTypeInfo info =
        {
            sizeof(IconComboClass), NULL, NULL,
            (GClassInitFunc) icon_combo_class_init, NULL, NULL,
            sizeof(IconCombo), 0,
            (GInstanceInitFunc) NULL,
        };

        type = g_type_register_static(COMBO_BOX_TYPE, "IconCombo", &info, 0);
    }

    return (type);
}

static void
emit_change(GtkWidget *button, IconCombo *ic)
{
    g_return_if_fail(ic != NULL);
    g_return_if_fail(0 <= ic->last_index);
    g_return_if_fail(ic->last_index < ic->num_elements);

    gtk_signal_emit(GTK_OBJECT(ic), icon_combo_signals[CHANGED],
                    ic->elements[ic->last_index].id);
}

static void
icon_clicked(GtkWidget *button, IconCombo *ic)
{
    int index = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "index"));
    icon_combo_select_icon_index(ic, index);
    emit_change(button, ic);
    combo_box_popup_hide(COMBO_BOX(ic));
}

static void
icon_table_setup(IconCombo *ic)
{
    int row, col, index = 0;

    ic->combo_table = gtk_table_new(ic->cols, ic->rows, 0);
    ic->tool_tip = gtk_tooltips_new();
    ic->icons = g_malloc(sizeof(GtkImage *) * ic->cols * ic->rows);

    for(row = 0; row < ic->rows; row++)
    {
        for(col = 0; col < ic->cols; ++col, ++index)
        {
            IconComboElement const *element = ic->elements + index;
            GtkWidget *button;

            if(element->stock_id == NULL)
            {
                /* exit both loops */
                row = ic->rows;
                break;
            }

            ic->icons[index] =
                gtk_image_new_from_stock(element->stock_id,
                                         GTK_ICON_SIZE_SMALL_TOOLBAR);
            button = gtk_button_new();
            gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
            gtk_container_add(GTK_CONTAINER(button),
                              GTK_WIDGET(ic->icons[index]));
            gtk_tooltips_set_tip(ic->tool_tip, button,
                                 _(element->untranslated_tooltip), NULL);

            gtk_table_attach(GTK_TABLE(ic->combo_table), button,
                             col, col + 1, row + 1, row + 2, GTK_FILL,
                             GTK_FILL, 1, 1);

            g_signal_connect(button, "clicked", G_CALLBACK(icon_clicked), ic);
            g_object_set_data(G_OBJECT(button), "index",
                              GINT_TO_POINTER(index));
        }
    }

    ic->num_elements = index;

    gtk_widget_show_all(ic->combo_table);
}

static void
icon_combo_construct(IconCombo *ic, IconComboElement const *elements,
                     int ncols, int nrows)
{
    g_return_if_fail(ic != NULL);
    g_return_if_fail(IS_ICON_COMBO(ic));

    /* Our table selector */
    ic->cols = ncols;
    ic->rows = nrows;
    ic->elements = elements;
    icon_table_setup(ic);

    ic->preview_button = gtk_button_new();
    gtk_button_set_relief(GTK_BUTTON(ic->preview_button), GTK_RELIEF_NONE);

    ic->preview_icon = gtk_image_new_from_stock(elements[0].stock_id,
                       GTK_ICON_SIZE_SMALL_TOOLBAR);

    gtk_container_add(GTK_CONTAINER(ic->preview_button),
                      GTK_WIDGET(ic->preview_icon));
    //  gtk_widget_set_usize (GTK_WIDGET (ic->preview_icon), 24, 24);
    g_signal_connect(ic->preview_button, "clicked",
                     G_CALLBACK(emit_change), ic);

    gtk_widget_show_all(ic->preview_button);

    combo_box_construct(COMBO_BOX(ic), ic->preview_button, ic->combo_table);
}

GtkWidget *
icon_combo_new(IconComboElement const *elements, int ncols, int nrows)
{
    IconCombo *ic;

    g_return_val_if_fail(elements != NULL, NULL);
    g_return_val_if_fail(elements != NULL, NULL);
    g_return_val_if_fail(ncols > 0, NULL);
    g_return_val_if_fail(nrows > 0, NULL);

    ic = g_object_new(ICON_COMBO_TYPE, NULL);
    icon_combo_construct(ic, elements, ncols, nrows);

    return (GTK_WIDGET(ic));
}

/* select a icon by its index */
static void
icon_combo_select_icon_index(IconCombo *ic, int index)
{
    g_return_if_fail(ic != NULL);
    g_return_if_fail(IS_ICON_COMBO(ic));
    g_return_if_fail(0 <= index);
    g_return_if_fail(index < ic->num_elements);

    ic->last_index = index;

    gtk_container_remove(GTK_CONTAINER(ic->preview_button),
                         ic->preview_icon);

    ic->preview_icon = gtk_image_new_from_stock(ic->elements[index].stock_id,
                       GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_widget_show(ic->preview_icon);

    gtk_container_add(GTK_CONTAINER(ic->preview_button), ic->preview_icon);
}

/* select a icon by its unique integer ID */
void
icon_combo_select_icon(IconCombo *ic, int id)
{
    int i;

    g_return_if_fail(ic != NULL);
    g_return_if_fail(IS_ICON_COMBO(ic));
    g_return_if_fail(ic->num_elements > 0);

    for(i = 0; i < ic->num_elements; i++)
        if(ic->elements[i].id == id)
        {
            break;
        }

    if(i >= ic->num_elements)
    {
        i = 0;
    }

    icon_combo_select_icon_index(ic, i);
}
