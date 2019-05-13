/*
 * SwamiguiPanel.c - Panel control interface type
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
#include <gtk/gtk.h>

#include "SwamiguiPanel.h"

static void swamigui_panel_interface_init (SwamiguiPanelIface *panel_iface);

GType
swamigui_panel_get_type (void)
{
  static GType itype = 0;

  if (!itype)
    {
      static const GTypeInfo info =
	{
	  sizeof (SwamiguiPanelIface),
	  NULL,			/* base_init */
	  NULL,			/* base_finalize */
	  (GClassInitFunc) swamigui_panel_interface_init,
	  (GClassFinalizeFunc) NULL
	};

      itype = g_type_register_static (G_TYPE_INTERFACE, "SwamiguiPanel",
				      &info, 0);
    }

  return (itype);
}

static void
swamigui_panel_interface_init (SwamiguiPanelIface *panel_iface)
{
  g_object_interface_install_property (panel_iface,
		g_param_spec_object ("item-selection", "Item Selection",
				     "Item selection list",
				     IPATCH_TYPE_LIST,
				     G_PARAM_READWRITE));
}

/**
 * swamigui_panel_type_get_info:
 * @type: Type with a #SwamiguiPanel interface to get info on
 * @label: Out - User panel label (short, could be %NULL)
 * @blurb: Out - User panel description (longer, for tool tips, could be %NULL)
 * @stockid: Out - Stock ID string of icon for this panel (could be %NULL)
 *
 * Lookup info on a panel for a given @type.  A %NULL value can be passed for any
 * of the string parameters to ignore it.  Note also that a %NULL value may
 * be returned for any of the return parameters.  The returned parameters are
 * internal and should not be modified or freed.
 */
void
swamigui_panel_type_get_info (GType type, char **label, char **blurb,
			      char **stockid)
{
  SwamiguiPanelIface *panel_iface;
  GObjectClass *klass;

  g_return_if_fail (g_type_is_a (type, SWAMIGUI_TYPE_PANEL));

  klass = g_type_class_ref (type);	/* ++ ref class */
  g_return_if_fail (klass != NULL);

  panel_iface = g_type_interface_peek (klass, SWAMIGUI_TYPE_PANEL);

  if (!panel_iface) g_type_class_unref (klass);	/* -- unref class */
  g_return_if_fail (panel_iface != NULL);

  if (label) *label = panel_iface->label;
  if (blurb) *blurb = panel_iface->blurb;
  if (stockid) *stockid = panel_iface->stockid;

  g_type_class_unref (klass);	/* -- unref class */
}

/**
 * swamigui_panel_type_check_selection:
 * @type: Type with a #SwamiguiPanel interface
 * @selection: Item selection to test support for, list should not be empty
 * @selection_types: 0 terminated array of unique item types in @selection or
 *   %NULL to calculate the array
 *
 * Checks if the panel with the given @type supports the item @selection.  The
 * @selection_types parameter is for optimization purposes, so that panel types
 * can quickly check if they should be active or not based on the item types
 * in the selection (possibly saving another iteration of the @selection), this
 * array will be calculated if not supplied, so its only useful if checking
 * many panel types for a given @selection.
 *
 * Returns: %TRUE if panel supports selection, %FALSE otherwise
 */
gboolean
swamigui_panel_type_check_selection (GType type, IpatchList *selection,
				     GType *selection_types)
{
  SwamiguiPanelIface *panel_iface;
  GType *free_selection_types = NULL;
  GObjectClass *klass;
  gboolean retval;

  g_return_val_if_fail (g_type_is_a (type, SWAMIGUI_TYPE_PANEL), FALSE);
  g_return_val_if_fail (IPATCH_IS_LIST (selection), FALSE);
  g_return_val_if_fail (selection->items != NULL, FALSE);

  klass = g_type_class_ref (type);	/* ++ ref class */
  g_return_val_if_fail (klass != NULL, FALSE);

  panel_iface = g_type_interface_peek (klass, SWAMIGUI_TYPE_PANEL);
  if (!panel_iface) g_type_class_unref (klass);	/* -- unref class */
  g_return_val_if_fail (panel_iface != NULL, FALSE);

  if (!panel_iface->check_selection)
  {
    g_type_class_unref (klass);		/* -- unref class */
    return (TRUE);
  }

  if (!selection_types)		/* ++ alloc */
    free_selection_types = swamigui_panel_get_types_in_selection (selection);

  retval = panel_iface->check_selection (selection, selection_types);

  g_free (free_selection_types);	/* -- free */

  g_type_class_unref (klass);		/* -- unref class */

  return (retval);
}

/**
 * swamigui_panel_get_types_in_selection:
 * @selection: Item selection list
 *
 * Gets an array of unique item types in @selection.
 *
 * Returns: Newly allocated and 0 terminated array of unique types of items
 * in @selection.
 */
GType *
swamigui_panel_get_types_in_selection (IpatchList *selection)
{
  GArray *typearray;
  GType type;
  GList *p;
  guint i;

  typearray = g_array_new (TRUE, FALSE, sizeof (GType));	/* ++ alloc */

  if (selection)
  {
    for (p = selection->items; p; p = p->next)
    {
      type = G_OBJECT_TYPE (p->data);

      for (i = 0; i < typearray->len; i++)
	if (g_array_index (typearray, GType, i) == type) break;

      if (i == typearray->len)
	g_array_append_val (typearray, type);
    }
  }

  return ((GType *)g_array_free (typearray, FALSE));	/* !! caller takes over allocation */
}
