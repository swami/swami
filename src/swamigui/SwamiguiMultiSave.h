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
#ifndef __SWAMIGUI_MULTI_SAVE_H__
#define __SWAMIGUI_MULTI_SAVE_H__

#include <gtk/gtk.h>
#include <libinstpatch/libinstpatch.h>

typedef struct _SwamiguiMultiSave SwamiguiMultiSave;
typedef struct _SwamiguiMultiSaveClass SwamiguiMultiSaveClass;

#define SWAMIGUI_TYPE_MULTI_SAVE   (swamigui_multi_save_get_type ())
#define SWAMIGUI_MULTI_SAVE(obj) \
  (GTK_CHECK_CAST ((obj), SWAMIGUI_TYPE_MULTI_SAVE, SwamiguiMultiSave))
#define SWAMIGUI_MULTI_SAVE_CLASS(klass) \
  (GTK_CHECK_CLASS_CAST ((klass), SWAMIGUI_TYPE_MULTI_SAVE, \
   SwamiguiMultiSaveClass))
#define SWAMIGUI_IS_MULTI_SAVE(obj) \
  (GTK_CHECK_TYPE ((obj), SWAMIGUI_TYPE_MULTI_SAVE))
#define SWAMIGUI_IS_MULTI_SAVE_CLASS(klass) \
  (GTK_CHECK_CLASS_TYPE ((klass), SWAMIGUI_TYPE_MULTI_SAVE))

/* multi item save dialog */
struct _SwamiguiMultiSave
{
    GtkDialog parent_instance;

    GtkListStore *store;		/* list store */
    guint flags;                  /* SwamiguiMultiSaveFlags */

    GtkWidget *accept_btn;        /* Save/Close button */
    GtkWidget *treeview;		/* tree view widget */
    GtkWidget *icon;		/* the icon of the dialog */
    GtkWidget *message;		/* message label at top of dialog */
    GtkWidget *scroll_win;	/* scroll window to put list in */
};

/* multi item save dialog class */
struct _SwamiguiMultiSaveClass
{
    GtkDialogClass parent_class;
};

/**
 * SwamiguiMultiSaveFlags:
 * @SWAMIGUI_MULTI_SAVE_CLOSE_MODE: Files will be closed upon dialog confirm and
 *   accept button is changed to a "Close" button.
 *
 * Some flags for use with swamigui_multi_save_new().
 */
typedef enum
{
    SWAMIGUI_MULTI_SAVE_CLOSE_MODE = 1 << 0
} SwamiguiMultiSaveFlags;

GType swamigui_multi_save_get_type(void);
GtkWidget *swamigui_multi_save_new(char *title, char *message, guint flags);
void swamigui_multi_save_set_selection(SwamiguiMultiSave *multi,
                                       IpatchList *selection);

#endif
