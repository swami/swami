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
#ifndef __SWAMIGUI_COMBO_ENTRY_H__
#define __SWAMIGUI_COMBO_ENTRY_H__

#include <gtk/gtk.h>
#include <libinstpatch/libinstpatch.h>

typedef struct _SwamiguiComboEntry SwamiguiComboEntry;
typedef struct _SwamiguiComboEntryClass SwamiguiComboEntryClass;

#define SWAMIGUI_TYPE_COMBO_ENTRY   (swamigui_combo_entry_get_type ())
#define SWAMIGUI_COMBO_ENTRY(obj) \
  (GTK_CHECK_CAST ((obj), SWAMIGUI_TYPE_COMBO_ENTRY, SwamiguiComboEntry))
#define SWAMIGUI_COMBO_ENTRY_CLASS(klass) \
  (GTK_CHECK_CLASS_CAST ((klass), SWAMIGUI_TYPE_COMBO_ENTRY, \
   SwamiguiComboEntryClass))
#define SWAMIGUI_IS_COMBO_ENTRY(obj) \
  (GTK_CHECK_TYPE ((obj), SWAMIGUI_TYPE_COMBO_ENTRY))
#define SWAMIGUI_IS_COMBO_ENTRY_CLASS(klass) \
  (GTK_CHECK_CLASS_TYPE ((klass), SWAMIGUI_TYPE_COMBO_ENTRY))

// Check if Gtk is 2.24.0 or greater (where "has entry" support and GtkComboBoxText was added)
#define GTK_COMBO_HAS_ENTRY     GTK_CHECK_VERSION(2, 24, 0)

/* Combo entry widget */
struct _SwamiguiComboEntry
{
#if GTK_COMBO_HAS_ENTRY
    GtkComboBoxText parent_instance;
#else
    GtkComboBoxEntry parent_instance;
#endif
};

/* Combo entry class */
struct _SwamiguiComboEntryClass
{
#if GTK_COMBO_HAS_ENTRY
    GtkComboBoxTextClass parent_class;
#else
    GtkComboBoxEntryClass parent_class;
#endif
};

GType swamigui_combo_entry_get_type(void);
GtkWidget *swamigui_combo_entry_new();

#endif

