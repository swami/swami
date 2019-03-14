/*
 * SwamiguiStatusbar.h - A statusbar (multiple labels/progresses)
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
#ifndef __SWAMIGUI_STATUSBAR_H__
#define __SWAMIGUI_STATUSBAR_H__

/* max chars for "Global" group status label item */
#define SWAMIGUI_STATUSBAR_GLOBAL_MAXLEN	24

#include <gtk/gtk.h>

typedef struct _SwamiguiStatusbar SwamiguiStatusbar;
typedef struct _SwamiguiStatusbarClass SwamiguiStatusbarClass;

#define SWAMIGUI_TYPE_STATUSBAR   (swamigui_statusbar_get_type ())
#define SWAMIGUI_STATUSBAR(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWAMIGUI_TYPE_STATUSBAR, SwamiguiStatusbar))
#define SWAMIGUI_STATUSBAR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), SWAMIGUI_TYPE_STATUSBAR, \
   SwamiguiStatusbarClass))
#define SWAMIGUI_IS_STATUSBAR(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWAMIGUI_TYPE_STATUSBAR))
#define SWAMIGUI_IS_STATUSBAR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), SWAMIGUI_TYPE_STATUSBAR))

/* Statusbar widget */
struct _SwamiguiStatusbar
{
  GtkFrame parent;

  /*< private >*/
  GtkWidget *box;	/* the hbox within the statusbar frame */
  GList *items;		/* active items (see StatusItem in .c) */
  guint id_counter;	/* unique status item ID counter */
  int default_timeout;	/* default timeout value in msecs */
};

/* Statusbar widget class */
struct _SwamiguiStatusbarClass
{
  GtkFrameClass parent_class;
};

/**
 * SwamiguiStatusbarCloseFunc:
 * @statusbar: The status bar widget
 * @widg: The message widget
 *
 * Callback function prototype which gets called when a close button on a
 * progress status bar item gets activated.
 *
 * Returns: Should return %TRUE to remove the item from the status bar, %FALSE
 *   to keep it (useful if a confirmation dialog is popped for the user, etc).
 */
typedef gboolean (*SwamiguiStatusbarCloseFunc)(SwamiguiStatusbar *statusbar,
					       GtkWidget *widg);

typedef enum
{
  SWAMIGUI_STATUSBAR_POS_LEFT,
  SWAMIGUI_STATUSBAR_POS_RIGHT
} SwamiguiStatusbarPos;

/* some special timeout values for statusbar messages */
typedef enum
{
  SWAMIGUI_STATUSBAR_TIMEOUT_DEFAULT = -1,  /* uses "default-timeout" property */
  SWAMIGUI_STATUSBAR_TIMEOUT_FOREVER = 0    /* don't timeout */
} SwamiguiStatusbarTimeout;

GType swamigui_statusbar_get_type (void);
GtkWidget *swamigui_statusbar_new (void);
guint swamigui_statusbar_add (SwamiguiStatusbar *statusbar, const char *group,
			      int timeout, guint pos, GtkWidget *widg);
void swamigui_statusbar_remove (SwamiguiStatusbar *statusbar, guint id,
				const char *group);
void swamigui_statusbar_printf (SwamiguiStatusbar *statusbar, const char *format,
				...) G_GNUC_PRINTF (2, 3);
GtkWidget *swamigui_statusbar_msg_label_new (const char *label, guint maxlen);
GtkWidget *swamigui_statusbar_msg_progress_new (const char *label,
					        SwamiguiStatusbarCloseFunc close);

void swamigui_statusbar_msg_set_timeout (SwamiguiStatusbar *statusbar, guint id,
					 const char *group, int timeout);
void swamigui_statusbar_msg_set_label (SwamiguiStatusbar *statusbar,
				       guint id, const char *group,
				       const char *label);
void swamigui_statusbar_msg_set_progress (SwamiguiStatusbar *statusbar,
					  guint id, const char *group, double val);

#endif
