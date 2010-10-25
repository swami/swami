/*
 * SwamiguiModEdit.c - User interface modulator editor widget
 *
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <libinstpatch/libinstpatch.h>
#include <libswami/SwamiControlProp.h>

#include "SwamiguiModEdit.h"
#include "SwamiguiControl.h"
#include "SwamiguiPanel.h"
#include "widgets/icon-combo.h"

#include "icons.h"
#include "util.h"
#include "i18n.h"

enum
{
  PROP_0,
  PROP_ITEM_SELECTION,
  PROP_MODULATORS
};

/* tree view list stuff */

enum
{
  DEST_LABEL,
  SRC_PIXBUF,
  SRC_LABEL,
  AMT_PIXBUF,
  AMT_LABEL,
  AMT_VALUE,
  MOD_PTR,			/* modulator pointer */
  NUM_FIELDS
};

/* destination combo box tree store fields */
enum
{
  DEST_COLUMN_TEXT,	/* Text to display for this group/generator */
  DEST_COLUMN_ID,	/* Index of group if group ID, generator ID otherwise */
  DEST_COLUMN_COUNT
};

/* flag set in DEST_COLUMN_ID for group items (unset for generators) */
#define DEST_COLUMN_ID_IS_GROUP		0x100

/* Modulator General Controller palette descriptions */
struct
{
  int ctrlnum;
  char *descr;
} modctrl_descr[] = {
  { IPATCH_SF2_MOD_CONTROL_NONE, N_("No Controller") },
  { IPATCH_SF2_MOD_CONTROL_NOTE_ON_VELOCITY, N_("Note-On Velocity") },
  { IPATCH_SF2_MOD_CONTROL_NOTE_NUMBER, N_("Note-On Key Number") },
  { IPATCH_SF2_MOD_CONTROL_POLY_PRESSURE, N_("Poly Pressure") },
  { IPATCH_SF2_MOD_CONTROL_CHAN_PRESSURE, N_("Channel Pressure") },
  { IPATCH_SF2_MOD_CONTROL_PITCH_WHEEL, N_("Pitch Wheel") },
  { IPATCH_SF2_MOD_CONTROL_BEND_RANGE, N_("Bend Range") }
};

#define MODCTRL_DESCR_COUNT \
    (sizeof (modctrl_descr) / sizeof (modctrl_descr[0]))

/* MIDI Continuous Controller descriptions */
struct
{
  int ctrlnum;
  char *descr;
} midicc_descr[] = {
  { 1, N_("Modulation") },
  { 2, N_("Breath Controller") },
  { 3, N_("Undefined") },
  { 4, N_("Foot Controller") },
  { 5, N_("Portamento Time") },
  { 7, N_("Main Volume") },
  { 8, N_("Balance") },
  { 9, N_("Undefined") },
  { 10, N_("Panpot") },
  { 11, N_("Expression Pedal") },
  { 12, N_("Effect Control 1") },
  { 13, N_("Effect Control 2") },
  { 14, N_("Undefined") },
  { 15, N_("Undefined") },
  { 16, N_("General Purpose 1") },
  { 17, N_("General Purpose 2") },
  { 18, N_("General Purpose 3") },
  { 19, N_("General Purpose 4") },

  /* 20-31 Undefined, 33-63 LSB for controllers 1-31 */

  { 64, N_("Hold 1 (Damper)") },
  { 65, N_("Portamento") },
  { 66, N_("Sostenuto") },
  { 67, N_("Soft Pedal") },
  { 68, N_("Undefined") },
  { 69, N_("Hold 2 (Freeze)") },

  /* 70-79 Undefined */

  { 80, N_("General Purpose 5") },
  { 81, N_("General Purpose 6") },
  { 82, N_("General Purpose 7") },
  { 83, N_("General Purpose 8") },

  /* 84-90 Undefined */

  { 91, N_("Effect 1 (Reverb)") },
  { 92, N_("Effect 2 (Tremolo)") },
  { 93, N_("Effect 3 (Chorus)") },
  { 94, N_("Effect 4 (Celeste)") },
  { 95, N_("Effect 5 (Phaser)") },
  { 96, N_("Data Increment") },
  { 97, N_("Data Decrement") }

  /* 102-119 Undefined */
};

#define MIDICC_DESCR_COUNT   (sizeof (midicc_descr) / sizeof (midicc_descr[0]))

char *modgroup_names[] = {
  N_("Sample"),
  N_("Pitch/Effects"),
  N_("Volume Envelope"),
  N_("Modulation Envelope"),
  N_("Modulation LFO"),
  N_("Vibrato LFO")
};

#define MODGROUP_COUNT   (sizeof (modgroup_names) / sizeof (modgroup_names[0]))

#define MODGROUP_SEPARATOR  (-1)

int modgroup_gens[] = {

  /* Sample group */

  IPATCH_SF2_GEN_SAMPLE_START,
  IPATCH_SF2_GEN_SAMPLE_COARSE_START,
  IPATCH_SF2_GEN_SAMPLE_END,
  IPATCH_SF2_GEN_SAMPLE_COARSE_END,
  IPATCH_SF2_GEN_SAMPLE_LOOP_START,
  IPATCH_SF2_GEN_SAMPLE_COARSE_LOOP_START,
  IPATCH_SF2_GEN_SAMPLE_LOOP_END,
  IPATCH_SF2_GEN_SAMPLE_COARSE_LOOP_END,

  MODGROUP_SEPARATOR,

  /* Pitch/Effects group */

  IPATCH_SF2_GEN_COARSE_TUNE,
  IPATCH_SF2_GEN_FINE_TUNE_OVERRIDE,
  IPATCH_SF2_GEN_FILTER_Q,
  IPATCH_SF2_GEN_FILTER_CUTOFF,
  IPATCH_SF2_GEN_REVERB,
  IPATCH_SF2_GEN_CHORUS,
  IPATCH_SF2_GEN_PAN,

  MODGROUP_SEPARATOR,

  /* Volumen Envelope group */

  IPATCH_SF2_GEN_VOL_ENV_DELAY,
  IPATCH_SF2_GEN_VOL_ENV_ATTACK,
  IPATCH_SF2_GEN_VOL_ENV_HOLD,
  IPATCH_SF2_GEN_VOL_ENV_DECAY,
  IPATCH_SF2_GEN_VOL_ENV_SUSTAIN,
  IPATCH_SF2_GEN_VOL_ENV_RELEASE,
  IPATCH_SF2_GEN_ATTENUATION,
  IPATCH_SF2_GEN_NOTE_TO_VOL_ENV_HOLD,
  IPATCH_SF2_GEN_NOTE_TO_VOL_ENV_DECAY,

  MODGROUP_SEPARATOR,

  /* Modulation Envelope group */

  IPATCH_SF2_GEN_MOD_ENV_DELAY,
  IPATCH_SF2_GEN_MOD_ENV_ATTACK,
  IPATCH_SF2_GEN_MOD_ENV_HOLD,
  IPATCH_SF2_GEN_MOD_ENV_DECAY,
  IPATCH_SF2_GEN_MOD_ENV_SUSTAIN,
  IPATCH_SF2_GEN_MOD_ENV_RELEASE,
  IPATCH_SF2_GEN_MOD_ENV_TO_PITCH,
  IPATCH_SF2_GEN_MOD_ENV_TO_FILTER_CUTOFF,
  IPATCH_SF2_GEN_NOTE_TO_MOD_ENV_HOLD,
  IPATCH_SF2_GEN_NOTE_TO_MOD_ENV_DECAY,

  MODGROUP_SEPARATOR,

  /* Modulation LFO group */

  IPATCH_SF2_GEN_MOD_LFO_DELAY,
  IPATCH_SF2_GEN_MOD_LFO_FREQ,
  IPATCH_SF2_GEN_MOD_LFO_TO_PITCH,
  IPATCH_SF2_GEN_MOD_LFO_TO_FILTER_CUTOFF,
  IPATCH_SF2_GEN_MOD_LFO_TO_VOLUME,

  MODGROUP_SEPARATOR,

  /* Vibrato LFO group */

  IPATCH_SF2_GEN_VIB_LFO_DELAY,
  IPATCH_SF2_GEN_VIB_LFO_FREQ,
  IPATCH_SF2_GEN_VIB_LFO_TO_PITCH,

  MODGROUP_SEPARATOR
};

#define MODGROUP_GENS_SIZE (sizeof (modgroup_gens) / sizeof (modgroup_gens[0]))

/* elements for source modulator transform icon combo widget */
static IconComboElement modtransform_elements[] = {
  { N_("Linear Positive Unipolar"), SWAMIGUI_STOCK_LINEAR_POS_UNI,
    IPATCH_SF2_MOD_TYPE_LINEAR | IPATCH_SF2_MOD_DIRECTION_POSITIVE | IPATCH_SF2_MOD_POLARITY_UNIPOLAR },
  { N_("Linear Negative Unipolar"), SWAMIGUI_STOCK_LINEAR_NEG_UNI,
    IPATCH_SF2_MOD_TYPE_LINEAR | IPATCH_SF2_MOD_DIRECTION_NEGATIVE | IPATCH_SF2_MOD_POLARITY_UNIPOLAR },
  { N_("Linear Positive Bipolar"), SWAMIGUI_STOCK_LINEAR_POS_BI,
    IPATCH_SF2_MOD_TYPE_LINEAR | IPATCH_SF2_MOD_DIRECTION_POSITIVE | IPATCH_SF2_MOD_POLARITY_BIPOLAR },
  { N_("Linear Negative Bipolar"), SWAMIGUI_STOCK_LINEAR_NEG_BI,
    IPATCH_SF2_MOD_TYPE_LINEAR | IPATCH_SF2_MOD_DIRECTION_NEGATIVE | IPATCH_SF2_MOD_POLARITY_BIPOLAR },

  { N_("Concave Positive Unipolar"), SWAMIGUI_STOCK_CONCAVE_POS_UNI,
    IPATCH_SF2_MOD_TYPE_CONCAVE | IPATCH_SF2_MOD_DIRECTION_POSITIVE | IPATCH_SF2_MOD_POLARITY_UNIPOLAR },
  { N_("Concave Negative Unipolar"), SWAMIGUI_STOCK_CONCAVE_NEG_UNI,
    IPATCH_SF2_MOD_TYPE_CONCAVE | IPATCH_SF2_MOD_DIRECTION_NEGATIVE | IPATCH_SF2_MOD_POLARITY_UNIPOLAR },
  { N_("Concave Positive Bipolar"), SWAMIGUI_STOCK_CONCAVE_POS_BI,
    IPATCH_SF2_MOD_TYPE_CONCAVE | IPATCH_SF2_MOD_DIRECTION_POSITIVE | IPATCH_SF2_MOD_POLARITY_BIPOLAR },
  { N_("Concave Negative Bipolar"), SWAMIGUI_STOCK_CONCAVE_NEG_BI,
    IPATCH_SF2_MOD_TYPE_CONCAVE | IPATCH_SF2_MOD_DIRECTION_NEGATIVE | IPATCH_SF2_MOD_POLARITY_BIPOLAR },

  { N_("Convex Positive Unipolar"), SWAMIGUI_STOCK_CONVEX_POS_UNI,
    IPATCH_SF2_MOD_TYPE_CONVEX | IPATCH_SF2_MOD_DIRECTION_POSITIVE | IPATCH_SF2_MOD_POLARITY_UNIPOLAR },
  { N_("Convex Negative Unipolar"), SWAMIGUI_STOCK_CONVEX_NEG_UNI,
    IPATCH_SF2_MOD_TYPE_CONVEX | IPATCH_SF2_MOD_DIRECTION_NEGATIVE | IPATCH_SF2_MOD_POLARITY_UNIPOLAR },
  { N_("Convex Positive Bipolar"), SWAMIGUI_STOCK_CONVEX_POS_BI,
    IPATCH_SF2_MOD_TYPE_CONVEX | IPATCH_SF2_MOD_DIRECTION_POSITIVE | IPATCH_SF2_MOD_POLARITY_BIPOLAR },
  { N_("Convex Negative Bipolar"), SWAMIGUI_STOCK_CONVEX_NEG_BI,
    IPATCH_SF2_MOD_TYPE_CONVEX | IPATCH_SF2_MOD_DIRECTION_NEGATIVE | IPATCH_SF2_MOD_POLARITY_BIPOLAR },

  { N_("Switch Positive Unipolar"), SWAMIGUI_STOCK_SWITCH_POS_UNI,
    IPATCH_SF2_MOD_TYPE_SWITCH | IPATCH_SF2_MOD_DIRECTION_POSITIVE | IPATCH_SF2_MOD_POLARITY_UNIPOLAR },
  { N_("Switch Negative Unipolar"), SWAMIGUI_STOCK_SWITCH_NEG_UNI,
    IPATCH_SF2_MOD_TYPE_SWITCH | IPATCH_SF2_MOD_DIRECTION_NEGATIVE | IPATCH_SF2_MOD_POLARITY_UNIPOLAR },
  { N_("Switch Positive Bipolar"), SWAMIGUI_STOCK_SWITCH_POS_BI,
    IPATCH_SF2_MOD_TYPE_SWITCH | IPATCH_SF2_MOD_DIRECTION_POSITIVE | IPATCH_SF2_MOD_POLARITY_BIPOLAR },
  { N_("Switch Negative Bipolar"), SWAMIGUI_STOCK_SWITCH_NEG_BI,
    IPATCH_SF2_MOD_TYPE_SWITCH | IPATCH_SF2_MOD_DIRECTION_NEGATIVE | IPATCH_SF2_MOD_POLARITY_BIPOLAR }
};

static void swamigui_mod_edit_class_init (SwamiguiModEditClass *klass);
static void swamigui_mod_edit_panel_iface_init (SwamiguiPanelIface *panel_iface);
static gboolean swamigui_mod_edit_panel_iface_check_selection (IpatchList *selection,
							       GType *selection_types);
static void swamigui_mod_edit_init (SwamiguiModEdit *modedit);
static void swamigui_mod_edit_set_property (GObject *object, guint property_id,
					    const GValue *value,
					    GParamSpec *pspec);
static void swamigui_mod_edit_get_property (GObject *object, guint property_id,
					    GValue *value, GParamSpec *pspec);
static void swamigui_mod_edit_finalize (GObject *object);
static GtkWidget *swamigui_mod_edit_create_list_view(SwamiguiModEdit *modedit);
static GtkTreeStore *swamigui_mod_edit_init_dest_combo_box (GtkWidget *combo_dest);
static void swamigui_mod_edit_cb_destination_changed (GtkComboBox *combo,
						      gpointer user_data);
static gboolean swamigui_mod_edit_real_set_selection (SwamiguiModEdit *modedit,
						      IpatchList *selection);
static gboolean swamigui_mod_edit_real_set_mods (SwamiguiModEdit *modedit,
						 IpatchSF2ModList *mods);
static void
swamigui_mod_edit_cb_mod_select_changed (GtkTreeSelection *selection,
					 SwamiguiModEdit *modedit);
static void swamigui_mod_edit_tree_selection_count (GtkTreeModel *model,
						    GtkTreePath *path,
						    GtkTreeIter *iter,
						    gpointer data);
static void swamigui_mod_edit_cb_new_clicked (GtkButton *btn,
					    SwamiguiModEdit *modedit);
static void swamigui_mod_edit_cb_delete_clicked (GtkButton *btn,
						 SwamiguiModEdit *modedit);
static void swamigui_mod_edit_selection_foreach (GtkTreeModel *model,
						 GtkTreePath *path,
						 GtkTreeIter *iter,
						 gpointer data);
static void swamigui_mod_edit_cb_pixcombo_changed (IconCombo *pixcombo, int id,
						   SwamiguiModEdit *modedit);
static void swamigui_mod_edit_cb_combo_list_select (GtkList *list,
						    GtkWidget *litem,
						    SwamiguiModEdit *modedit);
static void swamigui_mod_edit_cb_amtsrc_changed (GtkAdjustment *adj,
						 SwamiguiModEdit *modedit);
static void swamigui_mod_edit_add_source_combo_strings (GtkCombo *combo);
static void swamigui_mod_edit_update (SwamiguiModEdit *modedit);
static void swamigui_mod_edit_update_store_row (SwamiguiModEdit *modedit,
						GtkTreeIter *iter);
static void swamigui_mod_edit_set_active_mod (SwamiguiModEdit *modedit,
					      GtkTreeIter *iter,
					      gboolean force);
static gint swamigui_mod_edit_find_ctrl (gconstpointer a,
					 gconstpointer ctrlnum);
static char *swamigui_mod_edit_get_control_name (guint16 modsrc);
static char *swamigui_mod_edit_find_transform_icon (guint16 modsrc);
static int swamigui_mod_edit_find_gen_group (int genid, int *index);

static GObjectClass *parent_class = NULL;

GType
swamigui_mod_edit_get_type (void)
{
  static GType obj_type = 0;

  if (!obj_type)
    {
      static const GTypeInfo obj_info =
	{
	  sizeof (SwamiguiModEditClass), NULL, NULL,
	  (GClassInitFunc) swamigui_mod_edit_class_init, NULL, NULL,
	  sizeof (SwamiguiModEdit), 0,
	  (GInstanceInitFunc) swamigui_mod_edit_init,
	};
      static const GInterfaceInfo panel_info =
	{ (GInterfaceInitFunc)swamigui_mod_edit_panel_iface_init, NULL, NULL };

      obj_type = g_type_register_static (GTK_TYPE_SCROLLED_WINDOW,
					 "SwamiguiModEdit", &obj_info, 0);
      g_type_add_interface_static (obj_type, SWAMIGUI_TYPE_PANEL, &panel_info);
    }

  return (obj_type);
}

static void
swamigui_mod_edit_class_init (SwamiguiModEditClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  obj_class->set_property = swamigui_mod_edit_set_property;
  obj_class->get_property = swamigui_mod_edit_get_property;
  obj_class->finalize = swamigui_mod_edit_finalize;

  g_object_class_override_property (obj_class, PROP_ITEM_SELECTION, "item-selection");

  g_object_class_install_property (obj_class, PROP_MODULATORS,
		g_param_spec_boxed ("modulators", _("Modulators"),
				    _("Modulators"),
				    IPATCH_TYPE_SF2_MOD_LIST, G_PARAM_READWRITE));
}

static void
swamigui_mod_edit_panel_iface_init (SwamiguiPanelIface *panel_iface)
{
  panel_iface->label = _("Modulators");
  panel_iface->blurb = _("Edit real time effect controls");
  panel_iface->stockid = SWAMIGUI_STOCK_MODULATOR_EDITOR;
  panel_iface->check_selection = swamigui_mod_edit_panel_iface_check_selection;
}

static gboolean
swamigui_mod_edit_panel_iface_check_selection (IpatchList *selection,
					       GType *selection_types)
{ /* one item only and with mod item interface */
  return (!selection->items->next
	  && g_type_is_a (*selection_types, IPATCH_TYPE_SF2_MOD_ITEM));
}

static void
swamigui_mod_edit_set_property (GObject *object, guint property_id,
				const GValue *value, GParamSpec *pspec)
{
  SwamiguiModEdit *modedit = SWAMIGUI_MOD_EDIT (object);

  switch (property_id)
    {
    case PROP_ITEM_SELECTION:
      swamigui_mod_edit_real_set_selection (modedit, g_value_get_object (value));
      break;
    case PROP_MODULATORS:
      swamigui_mod_edit_real_set_mods (modedit,
			  (IpatchSF2ModList *)g_value_get_boxed (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
swamigui_mod_edit_get_property (GObject *object, guint property_id,
				GValue *value, GParamSpec *pspec)
{
  SwamiguiModEdit *modedit = SWAMIGUI_MOD_EDIT (object);

  switch (property_id)
    {
    case PROP_ITEM_SELECTION:
      g_value_set_object (value, modedit->selection);
      break;
    case PROP_MODULATORS:
      g_value_set_boxed (value, modedit->mods);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
swamigui_mod_edit_finalize (GObject *object)
{
  SwamiguiModEdit *modedit = SWAMIGUI_MOD_EDIT (object);

  g_object_unref (modedit->modctrl);
  if (modedit->selection) g_object_unref (modedit->selection);
  ipatch_sf2_mod_list_free (modedit->mods, TRUE);

  if (parent_class->finalize)
    parent_class->finalize (object);
}

static void
swamigui_mod_edit_init (SwamiguiModEdit *modedit)
{
  GtkWidget *glade_widg;
  GtkWidget *icon;
  GtkWidget *pixcombo;
  GtkWidget *widg;

  gtk_scrolled_window_set_hadjustment (GTK_SCROLLED_WINDOW (modedit), NULL);
  gtk_scrolled_window_set_vadjustment (GTK_SCROLLED_WINDOW (modedit), NULL);

  gtk_container_border_width (GTK_CONTAINER (modedit), 0);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (modedit),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  modedit->mod_selected = FALSE;
  modedit->block_callbacks = FALSE;

  /* create control for the "modulators" property and add to GUI queue */
  modedit->modctrl = SWAMI_CONTROL (swami_get_control_prop_by_name
				    (G_OBJECT (modedit), "modulators"));
  swamigui_control_set_queue (modedit->modctrl);

  glade_widg = swamigui_util_glade_create ("ModEdit");
  modedit->glade_widg = glade_widg;

  /* set up modulator tree view list widget */
  swamigui_mod_edit_create_list_view (modedit);

  /* configure callbacks on action buttons */
  widg = swamigui_util_glade_lookup (glade_widg, "BTNNew");
  g_signal_connect (widg, "clicked",
		    G_CALLBACK (swamigui_mod_edit_cb_new_clicked), modedit);

  widg = swamigui_util_glade_lookup (glade_widg, "BTNDel");
  g_signal_connect (widg, "clicked",
		    G_CALLBACK (swamigui_mod_edit_cb_delete_clicked), modedit);

  /* nice modulator junction icon */
  icon = gtk_image_new_from_stock (SWAMIGUI_STOCK_MODULATOR_JUNCT,
				   SWAMIGUI_ICON_SIZE_CUSTOM_LARGE1);
  gtk_widget_show (icon);
  widg = swamigui_util_glade_lookup (glade_widg, "HBXIcon");
  gtk_box_pack_start (GTK_BOX (widg), icon, FALSE, 0, 0);
  gtk_box_reorder_child (GTK_BOX (widg), icon, 0);

  /* create source modulator icon combo */
  pixcombo = icon_combo_new (modtransform_elements, 4, 4);
  gtk_widget_show (pixcombo);
  g_object_set_data (G_OBJECT (glade_widg), "PIXSrc", pixcombo);
  widg = swamigui_util_glade_lookup (glade_widg, "HBXSrc");
  gtk_box_pack_start (GTK_BOX (widg), pixcombo, FALSE, 0, 0);
  gtk_box_reorder_child (GTK_BOX (widg), pixcombo, 0);

  g_signal_connect (pixcombo, "changed",
		    G_CALLBACK (swamigui_mod_edit_cb_pixcombo_changed),
		    modedit);

  /* create amount source modulator icon combo */
  pixcombo = icon_combo_new (modtransform_elements, 4, 4);
  gtk_widget_show (pixcombo);
  g_object_set_data (G_OBJECT (glade_widg), "PIXAmtSrc", pixcombo);
  widg = swamigui_util_glade_lookup (glade_widg, "HBXAmtSrc");
  gtk_box_pack_start (GTK_BOX (widg), pixcombo, FALSE, 0, 0);
  gtk_box_reorder_child (GTK_BOX (widg), pixcombo, 0);

  g_signal_connect (pixcombo, "changed",
		    G_CALLBACK (swamigui_mod_edit_cb_pixcombo_changed),
		    modedit);

  /* add modulator source controller description strings to combos */
  widg = swamigui_util_glade_lookup (glade_widg, "COMSrcCtrl");
  swamigui_mod_edit_add_source_combo_strings (GTK_COMBO (widg));
  g_signal_connect (GTK_COMBO (widg)->list, "select-child",
		    G_CALLBACK (swamigui_mod_edit_cb_combo_list_select),
		    modedit);

  widg = swamigui_util_glade_lookup (glade_widg, "COMAmtCtrl");
  swamigui_mod_edit_add_source_combo_strings (GTK_COMBO (widg));
  g_signal_connect (GTK_COMBO (widg)->list, "select-child",
		    G_CALLBACK (swamigui_mod_edit_cb_combo_list_select),
		    modedit);

  /* add value changed signal to amount spin button */
  widg = swamigui_util_glade_lookup (glade_widg, "SPBAmount");
  g_signal_connect (gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (widg)),
		    "value-changed",
		    G_CALLBACK (swamigui_mod_edit_cb_amtsrc_changed), modedit);

  /* add generator groups to option menu */

  widg = swamigui_util_glade_lookup (glade_widg, "ComboDestination");
  modedit->dest_store = swamigui_mod_edit_init_dest_combo_box (widg);
  gtk_combo_box_set_model (GTK_COMBO_BOX (widg),
			   GTK_TREE_MODEL (modedit->dest_store));
  g_signal_connect (widg, "changed",
		    G_CALLBACK (swamigui_mod_edit_cb_destination_changed),
		    modedit);

  swamigui_mod_edit_set_active_mod (modedit, NULL, TRUE); /* disable editor */
  
  gtk_widget_show (glade_widg);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (modedit),
					 glade_widg);
}

static GtkWidget *
swamigui_mod_edit_create_list_view (SwamiguiModEdit *modedit)
{
  GtkWidget *tree;
  GtkTreeSelection *sel;
  GtkListStore *store;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  tree = swamigui_util_glade_lookup (modedit->glade_widg, "ModList");
  store = gtk_list_store_new (NUM_FIELDS,
			      G_TYPE_STRING,
			      GDK_TYPE_PIXBUF,
			      G_TYPE_STRING,
			      GDK_TYPE_PIXBUF,
			      G_TYPE_STRING,
			      G_TYPE_INT,
			      IPATCH_TYPE_SF2_MOD);

  gtk_tree_view_set_model (GTK_TREE_VIEW (tree), GTK_TREE_MODEL (store));

  modedit->tree_view = tree;
  modedit->list_store = store;

  sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));
  gtk_tree_selection_set_mode (sel, GTK_SELECTION_MULTIPLE);
  g_signal_connect (sel, "changed",
		    G_CALLBACK (swamigui_mod_edit_cb_mod_select_changed),
		    modedit);

  /* Disable tree view search, since it breaks piano key playback */
  g_object_set (tree, "enable-search", FALSE, NULL);

  /* destination label column */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Destination"),
						     renderer,
						     "text", DEST_LABEL,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

  /* source pixbuf and label column */
  renderer = gtk_cell_renderer_pixbuf_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Source"),
						     renderer,
						     "pixbuf", SRC_PIXBUF,
						     NULL);
  renderer = gtk_cell_renderer_text_new ();
  gtk_tree_view_column_pack_start (column, renderer, TRUE);
  gtk_tree_view_column_set_attributes (column, renderer,
				       "text", SRC_LABEL,
				       NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

  /* amount source pixbuf and label column */
  renderer = gtk_cell_renderer_pixbuf_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Amount Source"),
						     renderer,
						     "pixbuf", AMT_PIXBUF,
						     NULL);
  renderer = gtk_cell_renderer_text_new ();
  gtk_tree_view_column_pack_start (column, renderer, TRUE);
  gtk_tree_view_column_set_attributes (column, renderer,
				       "text", AMT_LABEL,
				       NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

  /* amount value column */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Amount"),
						     renderer,
						     "text", AMT_VALUE,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

  return (tree);
}

static GtkTreeStore *
swamigui_mod_edit_init_dest_combo_box (GtkWidget *combo_dest)
{
  GtkCellRenderer *renderer;
  GtkTreeStore *store;
  GtkTreeIter group_iter, gen_iter;
  int group, gen;
  char *name;

  store = gtk_tree_store_new (DEST_COLUMN_COUNT, G_TYPE_STRING, G_TYPE_INT);

  for (group = 0, gen = 0; group < MODGROUP_COUNT; group++)
  { /* append group name */
    gtk_tree_store_append (store, &group_iter, NULL);
    gtk_tree_store_set (store, &group_iter,
			DEST_COLUMN_TEXT, modgroup_names[group],
			DEST_COLUMN_ID, DEST_COLUMN_ID_IS_GROUP | group,
			-1);

    while (modgroup_gens[gen] != MODGROUP_SEPARATOR)
    {
      gtk_tree_store_append (store, &gen_iter, &group_iter);

      name = ipatch_sf2_gen_info[modgroup_gens[gen]].label;

      gtk_tree_store_set (store, &gen_iter,
			  DEST_COLUMN_TEXT, name,
			  DEST_COLUMN_ID, modgroup_gens[gen],
			  -1);
      gen++;
    }

    gen++;
  }

  renderer = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo_dest), renderer, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo_dest), renderer,
				  "text", DEST_COLUMN_TEXT,
				  NULL);
  return (store);
}

/**
 * swamigui_mod_edit_new:
 *
 * Create a new modulator editor object
 *
 * Returns: New widget of type SwamiguiModEdit
 */
GtkWidget *
swamigui_mod_edit_new (void)
{
  return (GTK_WIDGET (gtk_type_new (swamigui_mod_edit_get_type ())));
}

/**
 * swamigui_mod_edit_set_selection:
 * @modedit: Modulation editor object
  * @selection: Item selection to assign to modulator editor or %NULL (should
  *   currently contain one item only)
 *
 * Set item to edit modulator list of.  If @item does not implement the
 * #IpatchSF2ModItem interface then it is deactivated.
 */
void
swamigui_mod_edit_set_selection (SwamiguiModEdit *modedit, IpatchList *selection)
{
  if (swamigui_mod_edit_real_set_selection (modedit, selection))
    g_object_notify (G_OBJECT (modedit), "item-selection");
}

static gboolean
swamigui_mod_edit_real_set_selection (SwamiguiModEdit *modedit,
				      IpatchList *selection)
{
  GObject *item = NULL;

  g_return_val_if_fail (modedit != NULL, FALSE);
  g_return_val_if_fail (SWAMIGUI_IS_MOD_EDIT (modedit), FALSE);
  g_return_val_if_fail (!selection || IPATCH_IS_LIST (selection), FALSE);

  /* valid if single item and it implements the IpatchSF2ModItem interface */
  if (selection && selection->items && !selection->items->next
      && g_type_is_a (G_OBJECT_TYPE (selection->items->data),
		      IPATCH_TYPE_SF2_MOD_ITEM))
    item = G_OBJECT (selection->items->data);

  if (!item) selection = NULL;

  /* same item already selected? */
  if ((!selection && !modedit->selection)
      || (selection && modedit->selection
	  && selection->items->data == modedit->selection->items->data))
    return (FALSE);

  if (modedit->selection) g_object_unref (modedit->selection);

  if (selection) modedit->selection = g_object_ref (selection);
  else modedit->selection = NULL;

  /* disconnect any current connections to modulator editor "modulators" */
  swami_control_disconnect_all (modedit->modctrl);

  if (modedit->mods)
    {
      ipatch_sf2_mod_list_free (modedit->mods, TRUE);
      modedit->mods = NULL;
    }

  /* connect modulator editor to item "modulators" property */
  if (item)
    {
      swami_control_prop_connect_objects (G_OBJECT (item), "modulators",
					  G_OBJECT (modedit), NULL,
					  SWAMI_CONTROL_CONN_BIDIR);
      g_object_get (item, "modulators", &modedit->mods, NULL);
    }

  swamigui_mod_edit_update (modedit);

  return (TRUE);
}

/**
 * swamigui_mod_edit_set_mods:
 * @modedit: Modulation editor object
 * @mods: List of modulators to assign to modulator editor object, and thus its
 *   active item's modulators as well.  List is duplicated.
 *
 * Assign modulators to a modulator editor object and the item it is editing.
 */
void
swamigui_mod_edit_set_mods (SwamiguiModEdit *modedit, IpatchSF2ModList *mods)
{
  if (swamigui_mod_edit_real_set_mods (modedit, mods))
    g_object_notify ((GObject *)modedit, "modulators");
}

static gboolean
swamigui_mod_edit_real_set_mods (SwamiguiModEdit *modedit,
				 IpatchSF2ModList *mods)
{
  g_return_val_if_fail (SWAMIGUI_IS_MOD_EDIT (modedit), FALSE);

  if (modedit->mods)
    ipatch_sf2_mod_list_free (modedit->mods, TRUE);

  modedit->mods = ipatch_sf2_mod_list_duplicate (mods);
  swamigui_mod_edit_update (modedit);

  return (TRUE);
}

typedef struct
{
  int count;
  GtkTreeIter iter;
} ForeachSelBag;

/* callback for when a modulator gets selected in the list */
static void
swamigui_mod_edit_cb_mod_select_changed (GtkTreeSelection *selection,
					 SwamiguiModEdit *modedit)
{
  ForeachSelBag selbag;

  selbag.count = 0;

  /* count selection and get first selected iter */
  gtk_tree_selection_selected_foreach (selection,
				       swamigui_mod_edit_tree_selection_count,
				       &selbag);
  if (selbag.count == 1)
    swamigui_mod_edit_set_active_mod (modedit, &selbag.iter, FALSE);
  else				/* disable editor */
    swamigui_mod_edit_set_active_mod (modedit, NULL, TRUE);
}

static void
swamigui_mod_edit_tree_selection_count (GtkTreeModel *model, GtkTreePath *path,
					GtkTreeIter *iter, gpointer data)
{
  ForeachSelBag *selbag = (ForeachSelBag *)data;

  selbag->count += 1;
  if (selbag->count == 1) selbag->iter = *iter;
}

/* callback for new modulator button click */
static void
swamigui_mod_edit_cb_new_clicked (GtkButton *btn, SwamiguiModEdit *modedit)
{
  GtkTreeIter iter;
  GtkTreeSelection *selection;
  IpatchSF2Mod *mod;

  if (!modedit->selection) return;

  mod = ipatch_sf2_mod_new ();
  modedit->mods = ipatch_sf2_mod_list_insert (modedit->mods, mod, -1);

  g_object_notify ((GObject *)modedit, "modulators");

  gtk_list_store_append (modedit->list_store, &iter);
  gtk_list_store_set (modedit->list_store, &iter, MOD_PTR, mod, -1);

  ipatch_sf2_mod_free (mod);

  /* select the new item */
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (modedit->tree_view));
  gtk_tree_selection_unselect_all (selection);
  gtk_tree_selection_select_iter (selection, &iter);

  swamigui_mod_edit_update_store_row (modedit, &iter);
}

/* callback for delete modulator button click */
static void
swamigui_mod_edit_cb_delete_clicked (GtkButton *btn, SwamiguiModEdit *modedit)
{
  GtkTreeSelection *sel;
  GtkTreeIter *iter;
  GList *sel_iters = NULL, *p;
  IpatchSF2Mod *mod;
  gboolean changed;
  gboolean anychanged = FALSE;

  if (!modedit->selection) return;

  sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (modedit->tree_view));

  gtk_tree_selection_selected_foreach (sel,
				       swamigui_mod_edit_selection_foreach,
				       &sel_iters);
  p = sel_iters;
  while (p)
    {
      iter = (GtkTreeIter *)(p->data);

      gtk_tree_model_get (GTK_TREE_MODEL (modedit->list_store), iter,
			  MOD_PTR, &mod, -1);
      if (mod)
	{
	  modedit->mods = ipatch_sf2_mod_list_remove (modedit->mods, mod,
						      &changed);
	  if (changed) anychanged = TRUE;
	}

      gtk_list_store_remove (modedit->list_store, iter);

      ipatch_sf2_mod_free (mod); /* ## FREE mod from gtk_tree_model_get */
      gtk_tree_iter_free (iter);

      p = g_list_next (p);
    }

  g_list_free (sel_iters);

  if (anychanged) g_object_notify ((GObject *)modedit, "modulators");
}

/* a tree selection foreach callback to remove selected modulators */
static void
swamigui_mod_edit_selection_foreach (GtkTreeModel *model, GtkTreePath *path,
				     GtkTreeIter *iter, gpointer data)
{
  GList **sel_iters = (GList **)data;
  GtkTreeIter *copy;

  copy = gtk_tree_iter_copy (iter);
  *sel_iters = g_list_append (*sel_iters, copy);
}

/* callback when destination combo box is changed */
static void
swamigui_mod_edit_cb_destination_changed (GtkComboBox *combo, gpointer user_data)
{
  SwamiguiModEdit *modedit = SWAMIGUI_MOD_EDIT (user_data);
  IpatchSF2Mod *oldmod, newmod;
  GtkTreeIter iter, parent;
  GtkWidget *label;
  int groupid, genid;
  char *s;

  if (modedit->block_callbacks || !modedit->mod_selected) return;

  label = swamigui_util_glade_lookup (modedit->glade_widg, "LabelDestination");

  /* get active combo box item iterator and its parent group, return if none */
  if (!gtk_combo_box_get_active_iter (combo, &iter)
      || !gtk_tree_model_iter_parent (GTK_TREE_MODEL (modedit->dest_store),
				      &parent, &iter))
  {
    gtk_label_set_text (GTK_LABEL (label), "");
    return;
  }

  /* get the group ID value */
  gtk_tree_model_get (GTK_TREE_MODEL (modedit->dest_store), &parent,
		      DEST_COLUMN_ID, &groupid,
		      -1);

  groupid &= ~DEST_COLUMN_ID_IS_GROUP;	/* mask off the IS group flag */

  s = g_strdup_printf ("<b>%s</b>", modgroup_names[groupid]);
  gtk_label_set_markup (GTK_LABEL (label), s);
  g_free (s);


  /* get the generator ID of the selected item */
  gtk_tree_model_get (GTK_TREE_MODEL (modedit->dest_store), &iter,
		      DEST_COLUMN_ID, &genid,
		      -1);

  /* get current modulator values (gtk_tree_model_get duplicates mod) */
  gtk_tree_model_get (GTK_TREE_MODEL (modedit->list_store), &modedit->mod_iter,
		      MOD_PTR, &oldmod, -1);
  newmod = *oldmod;
  newmod.dest = genid;

  /* set the modulator values in the modulator list and notify property */
  if (ipatch_sf2_mod_list_change (modedit->mods, oldmod, &newmod))
    g_object_notify ((GObject *)modedit, "modulators");

  ipatch_sf2_mod_free (oldmod); /* free oldmod from gtk_tree_model_get */

  /* update the modulator in the list store */
  gtk_list_store_set (modedit->list_store, &modedit->mod_iter,
		      MOD_PTR, &newmod, -1);

  swamigui_mod_edit_update_store_row (modedit, &modedit->mod_iter);
}

/* callback for modulator source controller transform icon combo change */
static void
swamigui_mod_edit_cb_pixcombo_changed (IconCombo *pixcombo, int id,
				     SwamiguiModEdit *modedit)
{
  IpatchSF2Mod *oldmod, newmod;
  guint16 *src;

  if (modedit->block_callbacks || !modedit->mod_selected) return;

  /* get current modulator values (gtk_tree_model_get duplicates mod) */
  gtk_tree_model_get (GTK_TREE_MODEL (modedit->list_store), &modedit->mod_iter,
		      MOD_PTR, &oldmod, -1);
  newmod = *oldmod;		/* duplicate modulator values */

  if (pixcombo == g_object_get_data (G_OBJECT (modedit->glade_widg), "PIXSrc"))
    src = &newmod.src;
  else src = &newmod.amtsrc;

  *src &= ~(IPATCH_SF2_MOD_MASK_TYPE | IPATCH_SF2_MOD_MASK_DIRECTION
	    | IPATCH_SF2_MOD_MASK_POLARITY);
  *src |= id;

  /* set the modulator values in the modulator list and notify property */
  if (ipatch_sf2_mod_list_change (modedit->mods, oldmod, &newmod))
    g_object_notify ((GObject *)modedit, "modulators");

  ipatch_sf2_mod_free (oldmod); /* free oldmod from gtk_tree_model_get */

  /* update the modulator in the list store */
  gtk_list_store_set (modedit->list_store, &modedit->mod_iter,
		      MOD_PTR, &newmod, -1);

  swamigui_mod_edit_update_store_row (modedit, &modedit->mod_iter);
}

/* callback for modulator source controller combo list */
static void
swamigui_mod_edit_cb_combo_list_select (GtkList *list, GtkWidget *litem,
				      SwamiguiModEdit *modedit)
{
  IpatchSF2Mod *oldmod, newmod;
  GtkWidget *widg;
  guint16 *src;
  int ctrl;

  if (modedit->block_callbacks || !modedit->mod_selected) return;

  /* get current modulator values (gtk_tree_model_get duplicates mod) */
  gtk_tree_model_get (GTK_TREE_MODEL (modedit->list_store), &modedit->mod_iter,
		      MOD_PTR, &oldmod, -1);
  newmod = *oldmod;		/* duplicate modulator values */

  /* which source controller combo list? */
  widg = swamigui_util_glade_lookup (modedit->glade_widg, "COMSrcCtrl");
  if ((void *)list == (void *)(GTK_COMBO (widg)->list))
    src = &newmod.src;
  else src = &newmod.amtsrc;

  ctrl = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (litem), "ctrlnum"));
  *src &= ~(IPATCH_SF2_MOD_MASK_CONTROL | IPATCH_SF2_MOD_MASK_CC);
  *src |= ctrl;

  /* set the modulator values in the modulator list and notify property */
  if (ipatch_sf2_mod_list_change (modedit->mods, oldmod, &newmod))
    g_object_notify ((GObject *)modedit, "modulators");

  ipatch_sf2_mod_free (oldmod); /* free oldmod from gtk_tree_model_get */

  /* update the modulator in the list store */
  gtk_list_store_set (modedit->list_store, &modedit->mod_iter,
		      MOD_PTR, &newmod, -1);

  swamigui_mod_edit_update_store_row (modedit, &modedit->mod_iter);
}

/* callback for modulator amount source spin button value change */
static void
swamigui_mod_edit_cb_amtsrc_changed (GtkAdjustment *adj,
				     SwamiguiModEdit *modedit)
{
  IpatchSF2Mod *oldmod, newmod;

  if (modedit->block_callbacks || !modedit->mod_selected) return;

  /* get current modulator values (gtk_tree_model_get duplicates mod) */
  gtk_tree_model_get (GTK_TREE_MODEL (modedit->list_store), &modedit->mod_iter,
		      MOD_PTR, &oldmod, -1);
  newmod = *oldmod;		/* duplicate modulator values */

  newmod.amount = (gint16)(adj->value);

  /* set the modulator values in the modulator list and notify property */
  if (ipatch_sf2_mod_list_change (modedit->mods, oldmod, &newmod))
    g_object_notify ((GObject *)modedit, "modulators");

  ipatch_sf2_mod_free (oldmod); /* free oldmod from gtk_tree_model_get */

  /* update the modulator in the list store */
  gtk_list_store_set (modedit->list_store, &modedit->mod_iter,
		      MOD_PTR, &newmod, -1);

  swamigui_mod_edit_update_store_row (modedit, &modedit->mod_iter);
}

/* add modulator source controller descriptions to a combo list */
static void
swamigui_mod_edit_add_source_combo_strings (GtkCombo *combo)
{
  GtkWidget *item;
  char *descr, *s;
  int i, descrndx;

  /* add controls from the General Controller palette */
  for (i = 0; i < MODCTRL_DESCR_COUNT; i++)
    {
      item = gtk_list_item_new_with_label (_(modctrl_descr[i].descr));
      gtk_widget_show (item);

      g_object_set_data (G_OBJECT (item), "ctrlnum",
			 GINT_TO_POINTER (modctrl_descr[i].ctrlnum));

      gtk_container_add (GTK_CONTAINER (GTK_COMBO (combo)->list), item);
    }

  /* loop over all MIDI CC controllers up to last valid one (119) */
  for (i = 0, descrndx = 0; i < 120; i++)
    {
      if (midicc_descr[descrndx].ctrlnum == i)
	{
	  descr = _(midicc_descr[descrndx].descr);
	  if (descrndx < MIDICC_DESCR_COUNT - 1) descrndx++;
	}
      else if ((i >= 20 && i <= 31) ||
	       (i >= 70 && i <= 79) ||
	       (i >= 84 && i <= 90) ||
	       (i >= 102 && i <= 119))
	descr = _("Undefined");
      else descr = NULL;

      if (descr)
	{
	  s = g_strdup_printf (_("CC %d %s"), i, descr);
	  item = gtk_list_item_new_with_label (s);
	  gtk_widget_show (item);
	  g_free (s);

	  /* use the MIDI ctrl number with the modulator CC flag set */
	  g_object_set_data (G_OBJECT (item), "ctrlnum",
			     GINT_TO_POINTER (i | IPATCH_SF2_MOD_CC_MIDI));

	  gtk_container_add (GTK_CONTAINER (GTK_COMBO (combo)->list), item);
	}
    }
}

/* synchronizes modulator editor to current modulator list */
static void
swamigui_mod_edit_update (SwamiguiModEdit *modedit)
{
  GtkTreeIter iter;
  GSList *p;

  gtk_list_store_clear (modedit->list_store);

  swamigui_mod_edit_set_active_mod (modedit, NULL, FALSE); /* disable editor */

  if (!modedit->mods) return;

  for (p = modedit->mods; p; p = p->next)
    {
      gtk_list_store_append (modedit->list_store, &iter);
      gtk_list_store_set (modedit->list_store, &iter,
			  MOD_PTR, (IpatchSF2Mod *)(p->data), -1);
      swamigui_mod_edit_update_store_row (modedit, &iter);
    }
}

/* update a modulator in the list view */
static void
swamigui_mod_edit_update_store_row (SwamiguiModEdit *modedit,
				    GtkTreeIter *iter)
{
  GdkPixbuf *pixbuf;
  char *stock_id;
  int group;
  IpatchSF2Mod *mod;
  char *s;

  /* `mod' will be newly allocated */
  gtk_tree_model_get (GTK_TREE_MODEL (modedit->list_store), iter,
		      MOD_PTR, &mod, -1);

  /* set mod destination label */
  group = swamigui_mod_edit_find_gen_group (mod->dest, NULL);
  if (group >= 0)
    s = g_strdup_printf ("%s: %s", _(modgroup_names[group]),
			 _(ipatch_sf2_gen_info[mod->dest].label));
  else s = g_strdup_printf (_("Invalid (genid = %d)"), mod->dest);
  gtk_list_store_set (modedit->list_store, iter, DEST_LABEL, s, -1);
  g_free (s);

  /* set source pixbuf */
  stock_id = swamigui_mod_edit_find_transform_icon (mod->src);
  if (stock_id)
    {
      pixbuf = gtk_widget_render_icon (modedit->tree_view, stock_id,
				       GTK_ICON_SIZE_SMALL_TOOLBAR, NULL);
      gtk_list_store_set (modedit->list_store, iter, SRC_PIXBUF, pixbuf, -1);
    }

  /* set source label */
  s = swamigui_mod_edit_get_control_name (mod->src);
  gtk_list_store_set (modedit->list_store, iter, SRC_LABEL, s, -1);
  g_free (s);

  stock_id = swamigui_mod_edit_find_transform_icon (mod->amtsrc);
  if (stock_id)
    {
      pixbuf = gtk_widget_render_icon (modedit->tree_view, stock_id,
				       GTK_ICON_SIZE_SMALL_TOOLBAR, NULL);
      gtk_list_store_set (modedit->list_store, iter, AMT_PIXBUF, pixbuf, -1);
    }

  /* set amount source label */
  s = swamigui_mod_edit_get_control_name (mod->amtsrc);
  gtk_list_store_set (modedit->list_store, iter, AMT_LABEL, s, -1);
  g_free (s);

  /* set amount value */
  gtk_list_store_set (modedit->list_store, iter, AMT_VALUE, mod->amount, -1);

  ipatch_sf2_mod_free (mod);	/* ## FREE modulator from gtk_tree_model_get */
}

/* set the modulator that is being edited, or NULL to disable. */
static void
swamigui_mod_edit_set_active_mod (SwamiguiModEdit *modedit, GtkTreeIter *iter,
				  gboolean force)
{
  GtkWidget *pixsrc, *comsrc, *comdst, *lbldst, *spbamt, *pixamt, *comamt;
  GtkTreeIter destiter;
  IpatchSF2Mod *mod = NULL;
  GtkWidget *gw;
  GList *children, *found;
  int transform, ctrlnum, group, index;
  int pathcmp = 1;
  char *pathstr;
  char *s;

  /* get paths for new and current GtkTreeIter and compare them to see if
     request to set already set modulator */
  if (iter && modedit->mod_selected)
    {
      GtkTreePath *newpath, *curpath;

      newpath = gtk_tree_model_get_path (GTK_TREE_MODEL
					 (modedit->list_store), iter);
      curpath = gtk_tree_model_get_path (GTK_TREE_MODEL (modedit->list_store),
					 &modedit->mod_iter);
      pathcmp = gtk_tree_path_compare (newpath, curpath);
      gtk_tree_path_free (newpath);
      gtk_tree_path_free (curpath);
    }
  else if (!iter && !modedit->mod_selected) pathcmp = 0; /* already disabled */

  if (!force && pathcmp == 0) return;

  if (iter)
    {
      modedit->mod_selected = TRUE;
      modedit->mod_iter = *iter;
      gtk_tree_model_get (GTK_TREE_MODEL (modedit->list_store), iter,
			  MOD_PTR, &mod, -1);
    }
  else modedit->mod_selected = FALSE;

  gw = modedit->glade_widg;

  pixsrc = g_object_get_data (G_OBJECT (gw), "PIXSrc");
  comsrc = swamigui_util_glade_lookup (gw, "COMSrcCtrl");
  comdst = swamigui_util_glade_lookup (gw, "ComboDestination");
  lbldst = swamigui_util_glade_lookup (gw, "LabelDestination");
  spbamt = swamigui_util_glade_lookup (gw, "SPBAmount");
  pixamt = g_object_get_data (G_OBJECT (gw), "PIXAmtSrc");
  comamt = swamigui_util_glade_lookup (gw, "COMAmtCtrl");

  gtk_widget_set_sensitive (pixsrc, mod != NULL);
  gtk_widget_set_sensitive (comsrc, mod != NULL);
  gtk_widget_set_sensitive (comdst, mod != NULL);
  gtk_widget_set_sensitive (spbamt, mod != NULL);
  gtk_widget_set_sensitive (pixamt, mod != NULL);
  gtk_widget_set_sensitive (comamt, mod != NULL);

  modedit->block_callbacks = TRUE; /* block signal callbacks */

  /* set transform icon for source control */
  transform = mod ? mod->src & (IPATCH_SF2_MOD_MASK_TYPE
				| IPATCH_SF2_MOD_MASK_POLARITY
				| IPATCH_SF2_MOD_MASK_DIRECTION) : 0;
  icon_combo_select_icon (ICON_COMBO (pixsrc), transform);

  /* set control combo for source control */
  ctrlnum = mod ? mod->src & (IPATCH_SF2_MOD_MASK_CONTROL
			      | IPATCH_SF2_MOD_MASK_CC) : 0;
  children = gtk_container_children (GTK_CONTAINER (GTK_COMBO (comsrc)->list));
  found = g_list_find_custom (children, GINT_TO_POINTER (ctrlnum),
			      (GCompareFunc)swamigui_mod_edit_find_ctrl);
  if (found) gtk_list_select_child (GTK_LIST (GTK_COMBO (comsrc)->list),
				    GTK_WIDGET (found->data));
  else gtk_list_select_item (GTK_LIST (GTK_COMBO (comsrc)->list), 0);
  g_list_free (children);

  /* set destination generator group option menu */
  group = mod ? swamigui_mod_edit_find_gen_group (mod->dest, &index) : -1;
  if (group >= 0)
    { /* create group:index path string to select active combo box destination generator */
      pathstr = g_strdup_printf ("%d:%d", group, index);	/* ++ alloc */

      if (gtk_tree_model_get_iter_from_string (GTK_TREE_MODEL (modedit->dest_store),
					       &destiter, pathstr))
	gtk_combo_box_set_active_iter (GTK_COMBO_BOX (comdst), &destiter);

      g_free (pathstr);		/* -- free */

      s = g_strdup_printf ("<b>%s</b>", modgroup_names[group]);
      gtk_label_set_markup (GTK_LABEL (lbldst), s);
      g_free (s);
    }
  else
    {
      gtk_combo_box_set_active (GTK_COMBO_BOX (comdst), -1);
      gtk_label_set_text (GTK_LABEL (lbldst), "");
    }

  /* set amount spin button */
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spbamt), mod ? mod->amount : 0);

  /* set transform icon for amount source control */
  transform = mod ? mod->amtsrc & (IPATCH_SF2_MOD_MASK_TYPE
				   | IPATCH_SF2_MOD_MASK_POLARITY
				   | IPATCH_SF2_MOD_MASK_DIRECTION) : 0;
  icon_combo_select_icon (ICON_COMBO (pixamt), transform);

  /* set control combo for amount source control */
  ctrlnum = mod ? mod->amtsrc & (IPATCH_SF2_MOD_MASK_CONTROL
				 | IPATCH_SF2_MOD_MASK_CC) : 0;
  children = gtk_container_children (GTK_CONTAINER (GTK_COMBO (comamt)->list));
  found = g_list_find_custom (children, GINT_TO_POINTER (ctrlnum),
			      (GCompareFunc)swamigui_mod_edit_find_ctrl);
  if (found) gtk_list_select_child (GTK_LIST (GTK_COMBO (comamt)->list),
				    GTK_WIDGET (found->data));
  else gtk_list_select_item (GTK_LIST (GTK_COMBO (comamt)->list), 0);
  g_list_free (children);

  modedit->block_callbacks = FALSE; /* unblock callbacks */

  if (mod) ipatch_sf2_mod_free (mod);
}

/* a GCompareFunc for g_list_find_custom to locate a child GtkListItem in a
   list with a particular modulator control index */
static gint
swamigui_mod_edit_find_ctrl (gconstpointer a, gconstpointer ctrlnum)
{
  GtkListItem *litem = GTK_LIST_ITEM ((GtkListItem *)a);

  return (!(g_object_get_data (G_OBJECT (litem), "ctrlnum") == ctrlnum));
}

/* returns a description for the control of a modulator source enumeration,
   string should be freed when finished with */
static char *
swamigui_mod_edit_get_control_name (guint16 modsrc)
{
  int ctrlnum, i;

  ctrlnum = modsrc & IPATCH_SF2_MOD_MASK_CONTROL;

  if (modsrc & IPATCH_SF2_MOD_MASK_CC)
    {				/* MIDI CC controller */
      if ((ctrlnum >= 20 && ctrlnum <= 31) ||
	  (ctrlnum >= 70 && ctrlnum <= 79) ||
	  (ctrlnum >= 84 && ctrlnum <= 90) ||
	  (ctrlnum >= 102 && ctrlnum <= 119))
	return (g_strdup_printf (_("CC %d Undefined"), ctrlnum));

      /* loop over control descriptions */
      for (i = 0; i < MIDICC_DESCR_COUNT; i++)
	{
	  if (midicc_descr[i].ctrlnum == ctrlnum)
	    return (g_strdup_printf (_("CC %d %s"), ctrlnum,
				     _(midicc_descr[i].descr)));
	}
    }
  else
    { /* general modulator source controller */
      for (i = 0; i < MODCTRL_DESCR_COUNT; i++)
	{
	  if (modctrl_descr[i].ctrlnum == ctrlnum)
	    return (g_strdup (_(modctrl_descr[i].descr)));
	}
    }

  return (g_strdup_printf (_("Invalid (cc = %d, index = %d)"),
			   ((modsrc & IPATCH_SF2_MOD_MASK_CC) != 0), ctrlnum));
}

/* returns the icon stock ID for the transform type of the given modulator
   source enumeration or NULL if invalid */
static char *
swamigui_mod_edit_find_transform_icon (guint16 modsrc)
{
  int transform;
  int i;

  transform = modsrc & (IPATCH_SF2_MOD_MASK_TYPE | IPATCH_SF2_MOD_MASK_POLARITY
			| IPATCH_SF2_MOD_MASK_DIRECTION);

  for (i = 0; i < 16; i++)
    {
      if (modtransform_elements[i].id == transform)
	return (modtransform_elements[i].stock_id);
    }

  return (NULL);
}

/* determines the group a generator is part of and returns the group index
   or -1 if generator is not a valid modulator source, if index != NULL then
   the index within the group is stored in it */
static int
swamigui_mod_edit_find_gen_group (int genid, int *index)
{
  int group = 0;
  int i, groupndx = 0;

  for (i = 0; i < MODGROUP_GENS_SIZE; i++)
    {
      if (modgroup_gens[i] != MODGROUP_SEPARATOR)
	{
	  if (modgroup_gens[i] == genid) break;
	  groupndx++;
	}
      else			/* group separator */
	{
	  group++;
	  groupndx = 0;
	}
    }

  if (index) *index = groupndx;

  if (i < MODGROUP_GENS_SIZE)
    return (group);
  else return (-1);
}
