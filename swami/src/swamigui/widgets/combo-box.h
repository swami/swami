/* 
 * Ripped and slightly modified for Swami from libgal-0.19.2
 *
 * gtk-combo-box.h - a customizable combobox
 * Copyright 2000, 2001, Ximian, Inc.
 *
 * Authors:
 *   Miguel de Icaza <miguel@ximian.com>
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

#ifndef _COMBO_BOX_H_
#define _COMBO_BOX_H_

#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define COMBO_BOX_TYPE  (combo_box_get_type ())
#define COMBO_BOX(obj)	G_TYPE_CHECK_INSTANCE_CAST (obj, combo_box_get_type (), ComboBox)
#define COMBO_BOX_CLASS(klass)  \
    G_TYPE_CHECK_CLASS_CAST (klass, combo_box_get_type (), ComboBoxClass)
#define IS_COMBO_BOX(obj)  G_TYPE_CHECK_INSTANCE_TYPE (obj, combo_box_get_type ())

typedef struct _ComboBox	ComboBox;
typedef struct _ComboBoxPrivate ComboBoxPrivate;
typedef struct _ComboBoxClass   ComboBoxClass;

struct _ComboBox {
  GtkHBox hbox;
  ComboBoxPrivate *priv;
};

struct _ComboBoxClass {
  GtkHBoxClass parent_class;

  GtkWidget *(*pop_down_widget) (ComboBox *cbox);

  /*
   * invoked when the popup has been hidden, if the signal
   * returns TRUE, it means it should be killed from the
   */ 
  gboolean  *(*pop_down_done)   (ComboBox *cbox, GtkWidget *);

  /*
   * Notification signals.
   */
  void      (*pre_pop_down)     (ComboBox *cbox);
  void      (*post_pop_hide)    (ComboBox *cbox);
};

GType combo_box_get_type (void);
void combo_box_construct (ComboBox *combo_box, GtkWidget *display_widget,
			  GtkWidget *optional_pop_down_widget);
void combo_box_get_pos (ComboBox *combo_box, int *x, int *y);

GtkWidget *combo_box_new (GtkWidget *display_widget,
			  GtkWidget *optional_pop_down_widget);
void combo_box_popup_hide (ComboBox *combo_box);

void combo_box_set_display (ComboBox *combo_box, GtkWidget *display_widget);

void combo_box_set_title (ComboBox *combo, const gchar *title);

void combo_box_set_tearable (ComboBox *combo, gboolean tearable);
void combo_box_set_arrow_sensitive (ComboBox *combo, gboolean sensitive);
void combo_box_set_arrow_relief    (ComboBox *cc, GtkReliefStyle relief);
#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* _COMBO_BOX_H_ */
