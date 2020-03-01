/*
 * SwamiguiModEdit.h - User interface modulator editor object
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
#ifndef __SWAMIGUI_MOD_EDIT_H__
#define __SWAMIGUI_MOD_EDIT_H__

#include <gtk/gtk.h>
#include <libinstpatch/libinstpatch.h>
#include <libswami/SwamiControl.h>

typedef struct _SwamiguiModEdit SwamiguiModEdit;
typedef struct _SwamiguiModEditClass SwamiguiModEditClass;

#define SWAMIGUI_TYPE_MOD_EDIT   (swamigui_mod_edit_get_type ())
#define SWAMIGUI_MOD_EDIT(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWAMIGUI_TYPE_MOD_EDIT, SwamiguiModEdit))
#define SWAMIGUI_MOD_EDIT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), SWAMIGUI_TYPE_MOD_EDIT, \
   SwamiguiModEditClass))
#define SWAMIGUI_IS_MOD_EDIT(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWAMIGUI_TYPE_MOD_EDIT))
#define SWAMIGUI_IS_MOD_EDIT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), SWAMIGUI_TYPE_MOD_EDIT))

struct _SwamiguiModEdit
{
    GtkScrolledWindow parent;

    IpatchList *selection;	/* item selection or NULL (single item only) */
    IpatchSF2ModList *mods;	/* modulator list being edited (copy) */
    SwamiControl *modctrl;	/* "modulatos" property control */

    GtkWidget *tree_view;		/* tree view widget for modulator list */
    GtkListStore *list_store;	/* GtkTreeModel list store of modulator list */

    gboolean mod_selected;	/* modulator selected? (mod_iter is valid) */
    GtkTreeIter mod_iter;		/* modulator list node being edited */

    GtkWidget *glade_widg;	/* glade generated editor widget */
    gboolean block_callbacks;	/* blocks modulator editor callbacks */

    GtkTreeStore *dest_store;	/* destination combo box tree store */

    GtkListStore *src_store;      /* Source control list store */
};

struct _SwamiguiModEditClass
{
    GtkScrolledWindowClass parent_class;
};

GType swamigui_mod_edit_get_type(void);
GtkWidget *swamigui_mod_edit_new(void);
void swamigui_mod_edit_set_selection(SwamiguiModEdit *modedit,
                                     IpatchList *selection);
#endif
