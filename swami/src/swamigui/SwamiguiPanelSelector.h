/*
 * Swami
 * Copyright (C) 1999-2010 Joshua "Element" Green <jgreen@users.sourceforge.net>
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
/**
 * SECTION: SwamiguiPanelSelector
 * @short_description: Panel user interface notebook selection widget
 * @see_also: #SwamiguiPanel
 * @stability: Stable
 *
 * Notebook widget which provides access to valid user interface panels for a
 * given item selection.
 */
#ifndef __SWAMIGUI_PANEL_SELECTOR_H__
#define __SWAMIGUI_PANEL_SELECTOR_H__

#include <gtk/gtk.h>

typedef struct _SwamiguiPanelSelector SwamiguiPanelSelector;
typedef struct _SwamiguiPanelSelectorClass SwamiguiPanelSelectorClass;

#include "SwamiguiRoot.h"

#define SWAMIGUI_TYPE_PANEL_SELECTOR   (swamigui_panel_selector_get_type ())
#define SWAMIGUI_PANEL_SELECTOR(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWAMIGUI_TYPE_PANEL_SELECTOR, \
   SwamiguiPanelSelector))
#define SWAMIGUI_PANEL_SELECTOR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), SWAMIGUI_TYPE_PANEL_SELECTOR, \
   SwamiguiPanelSelectorClass))
#define SWAMIGUI_IS_PANEL_SELECTOR(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWAMIGUI_TYPE_PANEL_SELECTOR))
#define SWAMIGUI_IS_PANEL_SELECTOR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), SWAMIGUI_TYPE_PANEL_SELECTOR))

/* Panel user interface selection object */
struct _SwamiguiPanelSelector
{
  GtkNotebook parent;		/* derived from GtkNotebook */

  /*< private >*/
  IpatchList *selection;	/* Item selection */

  /* Active panel for each page (SwamiguiPanelInfo *), list does not own allocation */
  GList *active_panels;

  SwamiguiRoot *root;           /* Root object containing panel cache */
};

/* Panel selector object class */
struct _SwamiguiPanelSelectorClass
{
  GtkNotebookClass parent_class;
};

GType *swamigui_get_panel_selector_types (void);
void swamigui_register_panel_selector_type (GType panel_type, int order);

GType swamigui_panel_selector_get_type (void);
GtkWidget *swamigui_panel_selector_new (SwamiguiRoot *root);
void swamigui_panel_selector_set_selection (SwamiguiPanelSelector *selector,
					    IpatchList *items);
IpatchList *swamigui_panel_selector_get_selection (SwamiguiPanelSelector *selector);

#endif
