/*
 * SwamiguiPythonView.h - Header for python source viewer and shell.
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
#ifndef __SWAMIGUI_PYTHON_VIEW_H__
#define __SWAMIGUI_PYTHON_VIEW_H__

#include <gtk/gtk.h>
#include <libinstpatch/libinstpatch.h>

typedef struct _SwamiguiPythonView SwamiguiPythonView;
typedef struct _SwamiguiPythonViewClass SwamiguiPythonViewClass;

#define SWAMIGUI_TYPE_PYTHON_VIEW   (swamigui_python_view_get_type ())
#define SWAMIGUI_PYTHON_VIEW(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWAMIGUI_TYPE_PYTHON_VIEW, SwamiguiPythonView))
#define SWAMIGUI_PYTHON_VIEW_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), SWAMIGUI_TYPE_PYTHON_VIEW, \
   SwamiguiPythonViewClass))
#define SWAMIGUI_IS_PYTHON_VIEW(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWAMIGUI_TYPE_PYTHON_VIEW))
#define SWAMIGUI_IS_PYTHON_VIEW_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), SWAMIGUI_TYPE_PYTHON_VIEW))

/* Swami Python view/shell object */
struct _SwamiguiPythonView
{
  GtkVBox parent_instance;

  GtkWidget *glade_widg;	/* toplevel glade widget for python editor */
  GtkTextBuffer *srcbuf; /* source editor buffer (GtkSourceBuffer
			    or GtkTextBuffer depending on support libs) */
  GtkWidget *srcview;		/* source editor GtkSourceView */
  GtkTextBuffer *conbuf;	/* python output text buffer */
  GtkWidget *conview;		/* python console GtkTextBuffer */
  GtkWidget *comboscripts;	/* scripts combo box */
};

/* Swami Python view/shell class */
struct _SwamiguiPythonViewClass
{
  GtkVBoxClass parent_class;
};

GType swamigui_python_view_get_type (void);
GtkWidget *swamigui_python_view_new ();

#endif
