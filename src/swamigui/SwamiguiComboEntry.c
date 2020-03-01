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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include <gtk/gtk.h>

#include "SwamiguiComboEntry.h"
#include "i18n.h"

#if GTK_COMBO_HAS_ENTRY
static GObject *swamigui_combo_entry_constructor(GType gtype, guint n_properties,
        GObjectConstructParam *properties);
#endif

/* define the combo box entry type, depending on the Gtk version */
#if GTK_COMBO_HAS_ENTRY
G_DEFINE_TYPE(SwamiguiComboEntry, swamigui_combo_entry, GTK_TYPE_COMBO_BOX_TEXT);
#else
G_DEFINE_TYPE(SwamiguiComboEntry, swamigui_combo_entry, GTK_TYPE_COMBO_BOX_ENTRY);
#endif

static void
swamigui_combo_entry_class_init(SwamiguiComboEntryClass *klass)
{
#if GTK_COMBO_HAS_ENTRY
    GObjectClass *obj_class = G_OBJECT_CLASS(klass);

    obj_class->constructor = swamigui_combo_entry_constructor;
#endif
}

#if GTK_COMBO_HAS_ENTRY
static GObject *
swamigui_combo_entry_constructor(GType gtype, guint n_properties, GObjectConstructParam *properties)
{
    GObjectConstructParam *property;
    GObject *object;
    guint i;
    gchar const *name;

    // Kind of hackish no?  G_PARAM_CONSTRUCT_ONLY properties must be overriden here.
    for(i = 0, property = properties; i < n_properties; i++, property++)
    {
        name = g_param_spec_get_name(property->pspec);

        if(strcmp(name, "has-entry") == 0)
        {
            g_value_set_boolean(property->value, TRUE);
            break;
        }
    }

    object = G_OBJECT_CLASS(swamigui_combo_entry_parent_class)->constructor(gtype, n_properties, properties);

    g_object_set(object, "entry-text-column", 0, NULL);

    return object;
}
#endif

static void
swamigui_combo_entry_init(SwamiguiComboEntry *combo)
{
#if !GTK_COMBO_HAS_ENTRY
    g_object_set(combo, "text-column", 0, NULL);
#endif
}

/**
 * swamigui_combo_entry_new:
 *
 * Create a new combo box entry widget.
 *
 * Returns: New combo box entry widget.
 */
GtkWidget *
swamigui_combo_entry_new(void)
{
    return (GTK_WIDGET(g_object_new(SWAMIGUI_TYPE_COMBO_ENTRY, NULL)));
}

