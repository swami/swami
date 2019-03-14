/*
 * SwamiguiPanelSelector.c - Panel interface selection widget (notebook tabs)
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
#include <libinstpatch/libinstpatch.h>

#include "SwamiguiPanelSelector.h"
#include "SwamiguiPanel.h"
#include "i18n.h"

enum
{
  PROP_0,
  PROP_ITEM_SELECTION
};

/* Stores information on a registered panel interface */
typedef struct
{
  GType type;		/* Panel type */
  int order;		/* Sort order for this panel type */
} PanelInfo;


static void swamigui_panel_selector_set_property (GObject *object,
						  guint property_id,
						  const GValue *value,
						  GParamSpec *pspec);
static void swamigui_panel_selector_get_property (GObject *object,
						  guint property_id,
						  GValue *value,
						  GParamSpec *pspec);
static void swamigui_panel_selector_finalize (GObject *object);
static gboolean swamigui_panel_selector_button_press (GtkWidget *widget,
                                                      GdkEventButton *event);
static void swamigui_panel_selector_switch_page (GtkNotebook *notebook,
						 GtkWidget *page, guint page_num);
static gboolean swamigui_panel_selector_real_set_selection (SwamiguiPanelSelector *selector,
							    IpatchList *items);
static gint sort_panel_info_by_order (gconstpointer a, gconstpointer b);
static void swamigui_panel_selector_insert_panel (SwamiguiPanelSelector *selector,
						  PanelInfo *info, int pos);

G_DEFINE_TYPE (SwamiguiPanelSelector, swamigui_panel_selector, GTK_TYPE_NOTEBOOK);

static GList *panel_list = NULL;	/* list of registered panels (PanelInfo *) */
static guint panel_count = 0;		/* count of items in panel_list */

#define swamigui_panel_selector_info_new()	g_slice_new (PanelInfo)


/**
 * swamigui_get_panel_selector_types:
 *
 * Get array of GType widgets which implement the #SwamiguiPanel interface and
 * have been registered with swamigui_register_panel_selector_type().
 *
 * Returns: Array of GTypes (terminated with a 0 GType) which should be freed
 *   when finished, can be %NULL if empty list.
 */
GType *
swamigui_get_panel_selector_types (void)
{
  GType *types = NULL;
  PanelInfo *info;
  GList *p;
  guint len, i;

  if (!panel_list) return (NULL);

  len = panel_count;	/* atomic integer read, value increases only */
  if (len == 0) return (NULL);

  types = g_new (GType, len + 1);			/* ++ alloc */

  /* copy types */
  for (i = 0, p = panel_list; i < len; i++, p = p->next)
  {
    info = (PanelInfo *)(p->data);
    types[i] = info->type;
  }

  types[len] = 0;

  return (types);	/* !! caller takes over allocation */
}

/**
 * swamigui_register_panel_selector_type:
 * @panel_type: Type of widget with #SwamiguiPanel interface to register
 * @order: Order of the interface in relation to others (determines order of
 *   notepad tabs, lower values are placed left of higher values)
 *
 * Register a panel interface for use in the panel selector notebook widget.
 */
void
swamigui_register_panel_selector_type (GType panel_type, int order)
{
  PanelInfo *info;

  g_return_if_fail (g_type_is_a (panel_type, SWAMIGUI_TYPE_PANEL));

  info = swamigui_panel_selector_info_new ();
  info->type = panel_type;
  info->order = order;

  panel_list = g_list_append (panel_list, info);
  panel_count++;
}

static void
swamigui_panel_selector_class_init (SwamiguiPanelSelectorClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkNotebookClass *notebook_class = GTK_NOTEBOOK_CLASS (klass);

  obj_class->set_property = swamigui_panel_selector_set_property;
  obj_class->get_property = swamigui_panel_selector_get_property;
  obj_class->finalize = swamigui_panel_selector_finalize;

  widget_class->button_press_event = swamigui_panel_selector_button_press;

  notebook_class->switch_page = swamigui_panel_selector_switch_page;

  g_object_class_install_property (obj_class, PROP_ITEM_SELECTION,
		g_param_spec_object ("item-selection", _("Item selection"),
				     _("Item selection"),
				     IPATCH_TYPE_LIST, G_PARAM_READWRITE));
}

static void
swamigui_panel_selector_set_property (GObject *object, guint property_id,
				      const GValue *value, GParamSpec *pspec)
{
  SwamiguiPanelSelector *selector = SWAMIGUI_PANEL_SELECTOR (object);

  switch (property_id)
  {
  case PROP_ITEM_SELECTION:
    swamigui_panel_selector_real_set_selection (selector,
						g_value_get_object (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
swamigui_panel_selector_get_property (GObject *object, guint property_id,
				      GValue *value, GParamSpec *pspec)
{
  SwamiguiPanelSelector *selector = SWAMIGUI_PANEL_SELECTOR (object);

  switch (property_id)
  {
  case PROP_ITEM_SELECTION:
    g_value_set_object (value, selector->selection);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
swamigui_panel_selector_finalize (GObject *object)
{
  SwamiguiPanelSelector *selector = SWAMIGUI_PANEL_SELECTOR (object);

  g_list_free (selector->active_panels);

  if (selector->selection) g_object_unref (selector->selection);

  if (G_OBJECT_CLASS (swamigui_panel_selector_parent_class)->finalize)
    G_OBJECT_CLASS (swamigui_panel_selector_parent_class)->finalize (object);
}

static void
swamigui_panel_selector_init (SwamiguiPanelSelector *selector)
{
}

/* We focus the notebook widget so that it doesn't focus the first widget in the
 * child page which gets selected.  Very annoying when attempts to play piano
 * keyboard after switching notebook tabs causes the first entry widget to get
 * changed. */
static gboolean
swamigui_panel_selector_button_press (GtkWidget *widget, GdkEventButton *event)
{
  gtk_widget_grab_focus (widget);

  if (GTK_WIDGET_CLASS (swamigui_panel_selector_parent_class)->button_press_event)
    return (GTK_WIDGET_CLASS (swamigui_panel_selector_parent_class)->button_press_event
            (widget, event));
  else return (FALSE);
}

/* Switch page method which gets called when page is switched.
 * Sets the item selection of the panel for the new page. */
static void
swamigui_panel_selector_switch_page (GtkNotebook *notebook,
				     GtkWidget *page, guint page_num)
{
  SwamiguiPanelSelector *selector = SWAMIGUI_PANEL_SELECTOR (notebook);
  SwamiguiPanel *panel;
  GList *children;

  if (GTK_NOTEBOOK_CLASS (swamigui_panel_selector_parent_class)->switch_page)
    GTK_NOTEBOOK_CLASS (swamigui_panel_selector_parent_class)->switch_page
      (notebook, page, page_num);

  children = gtk_container_get_children (GTK_CONTAINER (notebook));	/* ++ alloc */

  panel = (SwamiguiPanel *)g_list_nth_data (children, page_num);

  if (panel) g_object_set (panel, "item-selection", selector->selection, NULL);

  g_list_free (children);	/* -- free */
}

/**
 * swamigui_panel_selector_new:
 *
 * Create panel selector notebook widget.
 *
 * Returns: New panel selector widget.
 */
GtkWidget *
swamigui_panel_selector_new (SwamiguiRoot *root)
{
  GtkWidget *widg;

  widg = (GtkWidget *)g_object_new (SWAMIGUI_TYPE_PANEL_SELECTOR, NULL);
  ((SwamiguiPanelSelector *)widg)->root = root;

  return (widg);
}

/**
 * swamigui_panel_selector_set_selection:
 * @editor: Panel selector widget
 * @items: List of selected items or %NULL to unset selection
 *
 * Set the item selection of a panel selector widget.
 */
void
swamigui_panel_selector_set_selection (SwamiguiPanelSelector *selector,
				       IpatchList *items)
{
  if (swamigui_panel_selector_real_set_selection (selector, items))
    g_object_notify (G_OBJECT (selector), "item-selection");
}

static gboolean
swamigui_panel_selector_real_set_selection (SwamiguiPanelSelector *selector,
					    IpatchList *selection)
{
  GList *old_panels;
  GtkWidget *panel;
  GType *item_types;
  PanelInfo *info;
  GList *children;
  GList *p;
  int i;

  g_return_val_if_fail (SWAMIGUI_IS_PANEL_SELECTOR (selector), FALSE);

  /* treat empty list as if NULL had been passed */
  if (selection && !selection->items) selection = NULL;

  if (!selection && !selector->selection) return (FALSE);

  if (selector->selection) g_object_unref (selector->selection);

  if (selection) selector->selection = ipatch_list_duplicate (selection);
  else selector->selection = NULL;

  old_panels = selector->active_panels;
  selector->active_panels = NULL;

  /* ++ alloc list - get list of current notebook children */
  children = gtk_container_get_children (GTK_CONTAINER (selector));

  if (selection)
  {
    /* get unique item types in items list (for optimization purposes) */
    item_types = swamigui_panel_get_types_in_selection (selection);	/* ++ alloc */

    /* loop over registered panel info */
    for (i = 0, p = panel_list; i < panel_count; i++, p = p->next)
    {
      info = (PanelInfo *)(p->data);

      /* add panel types to list for those which selection is valid */
      if (swamigui_panel_type_check_selection (info->type, selection, item_types))
	selector->active_panels = g_list_prepend (selector->active_panels, p->data);
    }

    g_free (item_types);	/* -- free */

    /* sort the list of active panel types (by their order value) */
    selector->active_panels = g_list_sort (selector->active_panels,
					   sort_panel_info_by_order);

    /* Add new panels (if any), may get pulled from cache instead of created */
    for (p = selector->active_panels, i = 0; p; p = p->next, i++)
    {
      info = (PanelInfo *)(p->data);

      /* panel not in old list - create a new panel or use cached one */
      if (!g_list_find (old_panels, info))
	swamigui_panel_selector_insert_panel (selector, info, i);
    }
  }

  /* Remove old unneeded panels and cache them */
  for (p = old_panels, i = 0; p; p = p->next, i++)
  {
    if (!g_list_find (selector->active_panels, p->data))
    {
      panel = g_list_nth_data (children, i);
      g_object_ref (panel);	/* ++ ref for cache */
      gtk_container_remove (GTK_CONTAINER (selector), GTK_WIDGET (panel));

      g_object_set (panel, "item-selection", NULL, NULL);

      if (selector->root)
        selector->root->panel_cache = g_list_prepend (selector->root->panel_cache, panel);
      else g_object_unref (panel);
    }
  }

  /* update "item-selection" for currently selected page */
  i = gtk_notebook_get_current_page (GTK_NOTEBOOK (selector));

  if (i != -1)
  {
    panel = gtk_notebook_get_nth_page (GTK_NOTEBOOK (selector), i);
    g_object_set (panel, "item-selection", selector->selection, NULL);
  }

  g_list_free (children);	/* -- free */
  g_list_free (old_panels);	/* -- free old panel info list */

  return (TRUE);
}

/* GCompareFunc for sorting list items by panel 'order' values */
static gint
sort_panel_info_by_order (gconstpointer a, gconstpointer b)
{
  PanelInfo *ainfo = (PanelInfo *)a, *binfo = (PanelInfo *)b;
  return (ainfo->order - binfo->order);
}

/* create a new panel (or use existing identical panel from cache) and add to
 * panel selector notebook at a given position */
static void
swamigui_panel_selector_insert_panel (SwamiguiPanelSelector *selector,
				      PanelInfo *info, int pos)
{
  GtkWidget *hbox, *widg;
  GtkWidget *panel = NULL;
  GtkWidget *cachepanel;
  char *label, *blurb, *stockid;
  PanelInfo *cacheinfo = NULL;
  GList *p;

  /* Check if there is already a panel of the requested type in the cache */
  if (selector->root)
  {
    for (p = selector->root->panel_cache; p; p = p->next)
    {
      cachepanel = (GtkWidget *)(p->data);
      cacheinfo = g_object_get_data (G_OBJECT (cachepanel), "_SwamiguiPanelInfo");

      if (cacheinfo == info)
      {
        panel = cachepanel;
        selector->root->panel_cache = g_list_delete_link (selector->root->panel_cache, p);
        break;
      }
    }
  }

  if (!panel)	/* Not found in cache? - Create it and associate with info. */
  {
    panel = GTK_WIDGET (g_object_new (info->type, NULL));
    g_object_set_data (G_OBJECT (panel), "_SwamiguiPanelInfo", info);
  }

  gtk_widget_show (panel);

  hbox = gtk_hbox_new (FALSE, 0);

  swamigui_panel_type_get_info (info->type, &label, &blurb, &stockid);

  if (stockid)
  {
    widg = gtk_image_new_from_stock (stockid, GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_box_pack_start (GTK_BOX (hbox), widg, FALSE, FALSE, 0);
  }

  if (label)
  {
    widg = gtk_label_new (label);
    gtk_box_pack_start (GTK_BOX (hbox), widg, FALSE, FALSE, 0);
  }

  if (blurb) gtk_widget_set_tooltip_text (GTK_WIDGET (hbox), blurb);

  gtk_widget_show_all (hbox);
  gtk_notebook_insert_page (GTK_NOTEBOOK (selector), panel, hbox, pos);

  /* -- unref the panel if it was taken from the cache */
  if (cacheinfo == info) g_object_unref (panel);
}

/**
 * swamigui_panel_selector_get_selection:
 * @selector: Panel selector widget
 *
 * Get the list of selected items for a panel selector widget.
 *
 * Returns: New list containing selected items which has a ref count of one
 *   which the caller owns or %NULL if no items selected. Remove the
 *   reference when finished with it.
 */
IpatchList *
swamigui_panel_selector_get_selection (SwamiguiPanelSelector *selector)
{
  g_return_val_if_fail (SWAMIGUI_IS_PANEL_SELECTOR (selector), NULL);

  if (selector->selection && selector->selection->items)
    return (ipatch_list_duplicate (selector->selection));
  else return (NULL);
}
