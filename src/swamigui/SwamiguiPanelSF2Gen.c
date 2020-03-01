/*
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
#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>

#include <libinstpatch/libinstpatch.h>
#include <libswami/libswami.h>

#include "SwamiguiControl.h"
#include "SwamiguiPanelSF2Gen.h"
#include "SwamiguiPanel.h"
#include "SwamiguiRoot.h"
#include "SwamiguiSpinScale.h"
#include "icons.h"
#include "util.h"
#include "i18n.h"

/* unit label used when generators are inactive */
#define BLANK_UNIT_LABEL  ""

/* structure used to keep track of generator widgets */
typedef struct
{
  GtkWidget *button;		/* value "set" toggle button */
  GtkWidget *spinscale;		/* SwamiguiSpinScale widget */
  GtkWidget *unitlabel;		/* unit label */
} GenWidgets;

/* value used for generator property selection type, IPATCH_SF2_GEN_PROPS_INST
 * and IPATCH_SF2_GEN_PROPS_PRESET values are also used */
#define SEL_NONE	-1

/* A separator (column or end) */
#define IS_SEPARATOR(genid) ((genid) >= SWAMIGUI_PANEL_SF2_GEN_COLUMN)

/* Is value from SwamiguiPanelSF2GenOp? */
#define IS_OP(genid) ((genid) >= SWAMIGUI_PANEL_SF2_GEN_LABEL)

enum
{
  PROP_0,
  PROP_ITEM_SELECTION
};

typedef struct _GenCtrl
{
  SwamiguiPanelSF2Gen *genpanel;	/* for GTK callback convenience */
  GtkWidget *scale;		/* scale widget */
  GtkWidget *entry;		/* entry widget */
  GtkWidget *units;		/* units label */
  GtkWidget *defbtn;		/* default toggle button */
  guint16 genid;
} GenCtrl;


static void swamigui_panel_sf2_gen_panel_iface_init (SwamiguiPanelIface *panel_iface);
static gboolean swamigui_sf2_ctrl_panel_iface_check_selection (IpatchList *selection,
							       GType *selection_types);
static void swamigui_panel_sf2_gen_set_property (GObject *object, guint property_id,
						const GValue *value,
						GParamSpec *pspec);
static void swamigui_panel_sf2_gen_get_property (GObject *object, guint property_id,
						GValue *value, GParamSpec *pspec);
static void swamigui_panel_sf2_gen_finalize (GObject *object);
static gboolean swamigui_panel_sf2_gen_real_set_selection (SwamiguiPanelSF2Gen *genpanel,
							   IpatchList *selection);


G_DEFINE_ABSTRACT_TYPE_WITH_CODE (SwamiguiPanelSF2Gen, swamigui_panel_sf2_gen,
                                  GTK_TYPE_SCROLLED_WINDOW,
		    G_IMPLEMENT_INTERFACE (SWAMIGUI_TYPE_PANEL,
					   swamigui_panel_sf2_gen_panel_iface_init));

/* Override mouse button event:
   To avoid lose of focus in pannels selector (GtkNoteBook tabs selector).
   Otherwise,the user would be forced to clic two times when he want to select
   another pannel.
*/
static gboolean swamigui_panel_sf2_gen_press_button (GtkWidget *widget,
                                                     GdkEventButton *event)
{
    /*  mouse click button propagation is ignored */
    return TRUE;
}

static void
swamigui_panel_sf2_gen_class_init (SwamiguiPanelSF2GenClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);

  /* Override mouse button event */
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  widget_class->button_press_event = swamigui_panel_sf2_gen_press_button ;

  obj_class->set_property = swamigui_panel_sf2_gen_set_property;
  obj_class->get_property = swamigui_panel_sf2_gen_get_property;
  obj_class->finalize = swamigui_panel_sf2_gen_finalize;

  g_object_class_override_property (obj_class, PROP_ITEM_SELECTION, "item-selection");
}

static void
swamigui_panel_sf2_gen_panel_iface_init (SwamiguiPanelIface *panel_iface)
{
  panel_iface->check_selection = swamigui_sf2_ctrl_panel_iface_check_selection;
}

static gboolean
swamigui_sf2_ctrl_panel_iface_check_selection (IpatchList *selection,
					       GType *selection_types)
{ /* One item only with IpatchSF2GenItem interface */
  return (!selection->items->next && g_type_is_a (G_OBJECT_TYPE (selection->items->data),
                                                  IPATCH_TYPE_SF2_GEN_ITEM));
}

static void
swamigui_panel_sf2_gen_set_property (GObject *object, guint property_id,
				     const GValue *value, GParamSpec *pspec)
{
  SwamiguiPanelSF2Gen *genpanel = SWAMIGUI_PANEL_SF2_GEN (object);

  switch (property_id)
    {
    case PROP_ITEM_SELECTION:
      swamigui_panel_sf2_gen_real_set_selection (genpanel,
						 g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
swamigui_panel_sf2_gen_get_property (GObject *object, guint property_id,
				     GValue *value, GParamSpec *pspec)
{
  SwamiguiPanelSF2Gen *genpanel = SWAMIGUI_PANEL_SF2_GEN (object);

  switch (property_id)
    {
    case PROP_ITEM_SELECTION:
      g_value_set_object (value, genpanel->selection);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
swamigui_panel_sf2_gen_finalize (GObject *object)
{
  SwamiguiPanelSF2Gen *genpanel = SWAMIGUI_PANEL_SF2_GEN (object);

  if (genpanel->selection) g_object_unref (genpanel->selection);

  G_OBJECT_CLASS (swamigui_panel_sf2_gen_parent_class)->finalize (object);
}

static void
swamigui_panel_sf2_gen_init (SwamiguiPanelSF2Gen *genpanel)
{
  genpanel->selection = NULL;
  genpanel->seltype = SEL_NONE;

  gtk_scrolled_window_set_hadjustment (GTK_SCROLLED_WINDOW (genpanel), NULL);
  gtk_scrolled_window_set_vadjustment (GTK_SCROLLED_WINDOW (genpanel), NULL);

  gtk_container_border_width (GTK_CONTAINER (genpanel), 0);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (genpanel),
    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
}

/**
 * swamigui_panel_sf2_gen_new:
 *
 * Create a new generator control panel object.
 *
 * Returns: new widget of type #SwamiguiPanelSF2Gen
 */
GtkWidget *
swamigui_panel_sf2_gen_new (void)
{
  return (GTK_WIDGET (gtk_type_new (swamigui_panel_sf2_gen_get_type ())));
}

/**
 * swamigui_panel_sf2_gen_set_controls:
 * @genpanel: Generator control panel
 * @ctrlinfo: Array of control info to configure panel with (should be static)
 *
 * Configure a SoundFont generator control panel from an array of control
 * info.  @ctrlinfo array should be terminated with a #SWAMIGUI_PANEL_SF2_GEN_END
 * genid item.
 */
void
swamigui_panel_sf2_gen_set_controls (SwamiguiPanelSF2Gen *genpanel,
                                     SwamiguiPanelSF2GenCtrlInfo *ctrlinfo)
{
  GtkWidget *hbox;
  GtkWidget *table;
  GtkWidget *widg;
  GtkWidget *image;
  GtkWidget *btn;
  GtkWidget *frame;
  GenWidgets *genwidgets;
  int i, ctrlndx, genid, row;
  SwamiguiPanelSF2GenCtrlInfo *ctrlp;
  /* get ipatch_sf2_gen_info from libinstpatch library */
  const IpatchSF2GenInfo * ipatch_sf2_gen_info = ipatch_sf2_get_gen_info();


  g_return_if_fail (SWAMIGUI_IS_PANEL_SF2_GEN (genpanel));
  g_return_if_fail (ctrlinfo != NULL);
  g_return_if_fail (genpanel->ctrlinfo == NULL);

  genpanel->ctrlinfo = ctrlinfo;
  genpanel->genwidget_count = 0;

  /* Count controls */
  for (ctrlp = ctrlinfo; ctrlp->genid != SWAMIGUI_PANEL_SF2_GEN_END; ctrlp++)
    if (!IS_OP (ctrlp->genid))
      genpanel->genwidget_count++;

  /* allocate array of gen widget structures */
  genwidgets = g_new (GenWidgets, genpanel->genwidget_count);
  genpanel->genwidgets = genwidgets;

  hbox = gtk_hbox_new (TRUE, 4);

  ctrlndx = 0;

next_column:

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);

  /* Get number of rows in this column */
  for (ctrlp = ctrlinfo, i = 0; !IS_SEPARATOR (ctrlp->genid); ctrlp++, i++);

  /* Create table for controls */
  table = gtk_table_new (i, 5, FALSE);
  gtk_container_add (GTK_CONTAINER (frame), table);

  /* loop over controls for this page */
  for (ctrlp = ctrlinfo, row = 0; !IS_SEPARATOR (ctrlp->genid); ctrlp++, row++)
  {
    genid = ctrlp->genid;

    if (genid == SWAMIGUI_PANEL_SF2_GEN_LABEL)
    {
      widg = gtk_label_new (NULL);
      gtk_label_set_markup (GTK_LABEL (widg), _(ctrlp->icon));
      gtk_table_attach (GTK_TABLE (table), widg, 0, 5, row, row + 1,
                        GTK_EXPAND | GTK_FILL, 0, 0, 0);
      continue;
    }

    /* generator icon */
    btn = gtk_toggle_button_new ();
    genwidgets[ctrlndx].button = btn;
    image = gtk_image_new_from_stock (ctrlp->icon, GTK_ICON_SIZE_MENU);
    gtk_button_set_image (GTK_BUTTON (btn), image);
    gtk_table_attach (GTK_TABLE (table), btn, 0, 1, row, row+1, 0, 0, 0, 0);

    /* create the control for the icon value set toggle button */
    swamigui_control_new_for_widget (G_OBJECT (btn));

    /* do this after control creation (since it mucks with it) */
    gtk_widget_set_sensitive (btn, FALSE);

    /* generator label */
    widg = gtk_label_new (_(ipatch_sf2_gen_info[genid].label));
    gtk_misc_set_alignment (GTK_MISC (widg), 0.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), widg, 1, 2, row, row+1, GTK_FILL, 0, 2, 0);

    /* create the horizontal scale and spin button combo widget */
    widg = swamigui_spin_scale_new ();
    genwidgets[ctrlndx].spinscale = widg;
    swamigui_spin_scale_set_order (SWAMIGUI_SPIN_SCALE (widg), TRUE);
    gtk_table_attach (GTK_TABLE (table), widg, 2, 3, row, row+1,
                      GTK_EXPAND | GTK_FILL, 0, 0, 0);

    gtk_entry_set_width_chars (GTK_ENTRY (SWAMIGUI_SPIN_SCALE (widg)->spinbtn), 8);

    /* create control for scale/spin combo widget */
    swamigui_control_new_for_widget (G_OBJECT (widg));	  

    /* do this after control creation (since it mucks with it) */
    gtk_widget_set_sensitive (widg, FALSE);

    /* units label */
    widg = gtk_label_new (BLANK_UNIT_LABEL);
    genwidgets[ctrlndx].unitlabel = widg;
    gtk_misc_set_alignment (GTK_MISC (widg), 0.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), widg, 4, 5, row, row+1, GTK_FILL, 0, 2, 0);

    ctrlndx++;
  }

  ctrlinfo = ctrlp + 1;

  if (ctrlp->genid == SWAMIGUI_PANEL_SF2_GEN_COLUMN)
    goto next_column;

  gtk_widget_show_all (hbox);
  gtk_container_border_width (GTK_CONTAINER (hbox), 4);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (genpanel), hbox);
}

/* the real selection set routine */
static gboolean
swamigui_panel_sf2_gen_real_set_selection (SwamiguiPanelSF2Gen *genpanel,
					   IpatchList *selection)
{
  IpatchSF2GenItemIface *geniface;
  IpatchSF2GenArray *genarray;
  GenWidgets *genwidgets;
  int seltype = SEL_NONE;
  SwamiControl *widgctrl, *propctrl;
  IpatchUnitInfo *unitinfo, *unituser;
  GObject *item;
  char *labeltext;
  int i, genid, unit, ctrlndx;
  SwamiguiPanelSF2GenCtrlInfo *ctrlp;

  g_return_val_if_fail (SWAMIGUI_IS_PANEL_SF2_GEN (genpanel), FALSE);
  g_return_val_if_fail (!selection || IPATCH_IS_LIST (selection), FALSE);

  /* short circuit if selection is NULL and already is NULL in panel */
  if (!genpanel->selection && (!selection || !selection->items)) return (FALSE);

  /* if selection with only one item and item has SF2 generator interface.. */
  if (selection && selection->items && !selection->items->next
      && IPATCH_IS_SF2_GEN_ITEM (selection->items->data))
    {	/* get generator property type for item */
      geniface = IPATCH_SF2_GEN_ITEM_GET_IFACE (selection->items->data);
      seltype = geniface->propstype & IPATCH_SF2_GEN_PROPS_MASK;
    }

  if (seltype == SEL_NONE)	/* force selection to NULL if not valid */
    {
      if (!genpanel->selection) return (FALSE);		/* short circuit if already NULL */
      selection = NULL;
    }

  if (selection == NULL)	/* inactive selection? */
    {
      genwidgets = (GenWidgets *)(genpanel->genwidgets);
      for (i = 0; i < genpanel->genwidget_count; i++, genwidgets++)
	{
	  widgctrl = swamigui_control_lookup (G_OBJECT (genwidgets->button));
	  swami_control_disconnect_all (widgctrl);

	  widgctrl = swamigui_control_lookup (G_OBJECT (genwidgets->spinscale));
	  swami_control_disconnect_all (widgctrl);

	  gtk_widget_set_sensitive (genwidgets->button, FALSE);
	  gtk_widget_set_sensitive (genwidgets->spinscale, FALSE);
	  gtk_label_set_text (GTK_LABEL (genwidgets->unitlabel), BLANK_UNIT_LABEL);
	}
    }
  else	/* selection is active */
    {
      item = G_OBJECT (selection->items->data);

      /* allocate generator array and fill with all gens from item */
      genarray = ipatch_sf2_gen_array_new (FALSE);	/* ++ alloc */
      ipatch_sf2_gen_item_copy_all (IPATCH_SF2_GEN_ITEM (item), genarray);

      genwidgets = (GenWidgets *)(genpanel->genwidgets);
      ctrlndx = 0;
      for (ctrlp = genpanel->ctrlinfo; ctrlp->genid != SWAMIGUI_PANEL_SF2_GEN_END; ctrlp++)
	{
          if (IS_OP (ctrlp->genid)) continue;

	  genid = ctrlp->genid;

	  widgctrl = swamigui_control_lookup (G_OBJECT (genwidgets->button));
	  swami_control_disconnect_all (widgctrl);

	  propctrl = swami_get_control_prop (item, geniface->setspecs[genid]);  // ++ ref
	  swami_control_connect (propctrl, widgctrl, SWAMI_CONTROL_CONN_BIDIR
				 | SWAMI_CONTROL_CONN_INIT);
          g_object_unref (propctrl);    // -- unref

	  widgctrl = swamigui_control_lookup (G_OBJECT (genwidgets->spinscale));
	  swami_control_disconnect_all (widgctrl);

	  propctrl = swami_get_control_prop (item, geniface->specs[genid]);  // ++ ref
          swami_control_connect_transform (propctrl, widgctrl, SWAMI_CONTROL_CONN_BIDIR_SPEC_INIT,
				           NULL, NULL, NULL, NULL, NULL, NULL);
          g_object_unref (propctrl);    // -- unref

          ipatch_param_get (geniface->specs[genid], "unit-type", &unit, NULL);

	  unitinfo = ipatch_unit_lookup (unit);

          if (unitinfo)
            unituser = ipatch_unit_class_lookup_map (IPATCH_UNIT_CLASS_USER, unitinfo->id);
          else unituser = NULL;

          swamigui_spin_scale_set_transform (SWAMIGUI_SPIN_SCALE (genwidgets->spinscale),
                                             unituser ? unit : IPATCH_UNIT_TYPE_NONE,
                                             unituser ? unituser->id : IPATCH_UNIT_TYPE_NONE);

	  gtk_widget_set_sensitive (genwidgets->button, TRUE);
	  gtk_widget_set_sensitive (genwidgets->spinscale, TRUE);


          if (unitinfo)
          {
            labeltext = unituser ? unituser->label : unitinfo->label;
            gtk_label_set_text (GTK_LABEL (genwidgets->unitlabel), labeltext);
          }

          ctrlndx++;
          genwidgets++;
        }
    }

  if (genpanel->selection) g_object_unref (genpanel->selection);

  if (selection)
    genpanel->selection = (IpatchList *)ipatch_list_duplicate (selection);
  else genpanel->selection = NULL;

  genpanel->seltype = seltype;

  return (TRUE);
}
