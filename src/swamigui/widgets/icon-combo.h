/* 
 * Ripped and modified for Swami from libgal-0.19.2
 *
 * widget-pixmap-combo.h - A icon selector combo box
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

#ifndef __ICON_COMBO_H__
#define __ICON_COMBO_H__

#include <gtk/gtkwidget.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkpixmap.h>
#include <gtk/gtktooltips.h>
#include "combo-box.h"

#define ICON_COMBO_TYPE     (icon_combo_get_type ())
#define ICON_COMBO(obj)     (GTK_CHECK_CAST((obj), ICON_COMBO_TYPE, IconCombo))
#define ICON_COMBO_CLASS(k) (GTK_CHECK_CLASS_CAST(k), ICON_COMBO_TYPE)
#define IS_ICON_COMBO(obj)  (GTK_CHECK_TYPE((obj), ICON_COMBO_TYPE))

typedef struct
{
  char const *untranslated_tooltip;
  char *stock_id;		/* icon stock ID */
  int id;
} IconComboElement;

typedef struct
{
  ComboBox     combo_box;

  /* Static information */
  IconComboElement const *elements;
  int cols, rows;
  int num_elements;

  /* State info */
  int last_index;

  /* Interface elements */
  GtkWidget    *combo_table, *preview_button;
  GtkWidget    *preview_icon;
  GtkTooltips  *tool_tip;
  GtkWidget **icons;		/* icon widgets */
} IconCombo;

GType icon_combo_get_type (void);
GtkWidget *icon_combo_new (IconComboElement const *elements,
			   int ncols, int nrows);
void icon_combo_select_icon (IconCombo *combo, int id);

typedef struct
{
  ComboBoxClass parent_class;

  /* Signals emited by this widget */
  void (* changed) (IconCombo *icon_combo, int id);
} IconComboClass;

#endif /* __ICON_COMBO_H__ */
