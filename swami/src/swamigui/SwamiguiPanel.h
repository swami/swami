/*
 * SwamiguiPanel.h - Panel control interface type
 * For managing control interfaces in a plug-able way.
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
#ifndef __SWAMIGUI_PANEL_H__
#define __SWAMIGUI_PANEL_H__

typedef struct _SwamiguiPanel SwamiguiPanel;		/* dummy typedef */
typedef struct _SwamiguiPanelIface SwamiguiPanelIface;

#include <gtk/gtk.h>
#include <libinstpatch/IpatchList.h>

#define SWAMIGUI_TYPE_PANEL   (swamigui_panel_get_type ())
#define SWAMIGUI_PANEL(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWAMIGUI_TYPE_PANEL, SwamiguiPanel))
#define SWAMIGUI_PANEL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), SWAMIGUI_TYPE_PANEL, SwamiguiPanelIface))
#define SWAMIGUI_IS_PANEL(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWAMIGUI_TYPE_PANEL))
#define SWAMIGUI_PANEL_GET_IFACE(obj) \
  (G_TYPE_INSTANCE_GET_INTERFACE ((obj), SWAMIGUI_TYPE_PANEL, \
   SwamiguiPanelIface))

/**
 * SwamiguiPanelCheckFunc:
 * @selection: Item selection to verify (will contain at least 1 item)
 * @selection_types: 0 terminated array of unique item GTypes found in @selection
 *
 * Function prototype used for checking if an item selection is valid for a
 * panel.
 *
 * Returns: Should return %TRUE if item selection is valid, %FALSE otherwise
 */
typedef gboolean (*SwamiguiPanelCheckFunc)(IpatchList *selection,
					   GType *selection_types);

struct _SwamiguiPanelIface
{
  GTypeInterface parent_class;

  char *label;			/* User label name for panel */
  char *blurb;			/* more descriptive text about panel */
  char *stockid;		/* stock ID of icon */

  SwamiguiPanelCheckFunc check_selection;
};

GType swamigui_panel_get_type (void);
void swamigui_panel_type_get_info (GType type, char **label, char **blurb,
				   char **stockid);
gboolean swamigui_panel_type_check_selection (GType type, IpatchList *selection,
					      GType *selection_types);
GType *swamigui_panel_get_types_in_selection (IpatchList *selection);

#endif
