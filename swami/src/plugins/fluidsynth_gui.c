/*
 * fluidsynth_gui.c - GUI widgets for FluidSynth Swami plugin.
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
#include "config.h"

#include <stdio.h>
#include <string.h>

#include <libinstpatch/libinstpatch.h>
#include <swamigui/swamigui.h>
#include "fluidsynth_gui_i18n.h"


typedef struct
{
  GtkVBox parent_instance;
  GtkWidget *ctrl_widg;
} FluidSynthGuiControl;

typedef struct
{
  GtkVBoxClass parent_class;
} FluidSynthGuiControlClass;


static int plugin_fluidsynth_gui_init (SwamiPlugin *plugin, GError **err);
static GtkWidget *fluid_synth_pref_handler (void);
static void fluid_synth_gui_audio_driver_changed (GtkComboBox *combo,
						  gpointer user_data);
static void fluid_synth_gui_midi_driver_changed (GtkComboBox *combo,
						 gpointer user_data);
static GType fluid_synth_gui_control_register_type (SwamiPlugin *plugin);
static void fluid_synth_gui_control_class_init (FluidSynthGuiControlClass *klass);
static void fluid_synth_gui_control_init (FluidSynthGuiControl *fsctrl);

SWAMI_PLUGIN_INFO (plugin_fluidsynth_gui_init, NULL);


/* List of audio and MIDI drivers that have Glade widgets */
const char *audio_driver_widgets[] = { "alsa", "jack", "oss", "dsound", NULL };
const char *midi_driver_widgets[] = { "alsa_seq", "alsa_raw", "oss", NULL };


/* --- functions --- */


/* plugin init function (one time initialize of SwamiPlugin) */
static gboolean
plugin_fluidsynth_gui_init (SwamiPlugin *plugin, GError **err)
{
  /* bind the gettext domain */
#if defined(ENABLE_NLS)
  bindtextdomain ("SwamiPlugin-fluidsynth-gui", LOCALEDIR);
#endif

  g_object_set (plugin,
		"name", "FluidSynthGui",
		"version", "1.01",
		"author", "Josh Green",
		"copyright", "Copyright (C) 2007-2008",
		"descr", N_("FluidSynth software wavetable synth GUI plugin"),
		"license", "GPL",
		NULL);

  /* initialize types */
  fluid_synth_gui_control_register_type (plugin);
//  fluid_synth_gui_map_register_type (plugin);
//  fluid_synth_gui_channels_register_type (plugin);

  swamigui_register_pref_handler ("FluidSynth", GTK_STOCK_MEDIA_PLAY,
				  SWAMIGUI_PREF_ORDER_NAME,
				  fluid_synth_pref_handler);
  return (TRUE);
}

/* preferences handler */
static GtkWidget *
fluid_synth_pref_handler (void)
{
  GtkWidget *fluid_widg;
  GtkWidget *widg;
  GtkListStore *store;
  GtkCellRenderer *cell;
  char **options, **optionp;

  fluid_widg = swamigui_util_glade_create ("FluidSynthPrefs");

  if (swamigui_root->wavetbl)
  {
    /* Initialize audio driver list */
    widg = swamigui_util_glade_lookup (fluid_widg, "ComboAudioDriver");

    store = gtk_list_store_new (1, G_TYPE_STRING);
    gtk_combo_box_set_model (GTK_COMBO_BOX (widg), GTK_TREE_MODEL (store));
    g_object_unref (store);

    cell = gtk_cell_renderer_text_new ();
    gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (widg), cell, TRUE);
    gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (widg), cell,
				    "text", 0,
				    NULL);

    g_object_get (swamigui_root->wavetbl, "audio-driver-options", &options, NULL);	/* ++ alloc */

    for (optionp = options; *optionp; optionp++)
      gtk_combo_box_append_text (GTK_COMBO_BOX (widg), *optionp);

    g_boxed_free (G_TYPE_STRV, options);	/* -- free */

    /* Connect to changed signal of audio driver combo box */
    g_signal_connect (widg, "changed",
		      G_CALLBACK (fluid_synth_gui_audio_driver_changed), fluid_widg);

    /* Connect the audio combo box to the "audio.driver" property */
    swamigui_control_prop_connect_widget (G_OBJECT (swamigui_root->wavetbl),
					  "audio.driver", G_OBJECT (widg));

    /* Initialize MIDI driver list */
    widg = swamigui_util_glade_lookup (fluid_widg, "ComboMidiDriver");

    store = gtk_list_store_new (1, G_TYPE_STRING);
    gtk_combo_box_set_model (GTK_COMBO_BOX (widg), GTK_TREE_MODEL (store));
    g_object_unref (store);

    cell = gtk_cell_renderer_text_new ();
    gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (widg), cell, TRUE);
    gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (widg), cell,
				    "text", 0,
				    NULL);

    g_object_get (swamigui_root->wavetbl, "midi-driver-options", &options, NULL);	/* ++ alloc */

    for (optionp = options; *optionp; optionp++)
      gtk_combo_box_append_text (GTK_COMBO_BOX (widg), *optionp);

    g_boxed_free (G_TYPE_STRV, options);	/* -- free */

    /* Connect to changed signal of MIDI driver combo box */
    g_signal_connect (widg, "changed",
		      G_CALLBACK (fluid_synth_gui_midi_driver_changed), fluid_widg);

    /* Connect the MIDI combo box to the "midi.driver" property */
    swamigui_control_prop_connect_widget (G_OBJECT (swamigui_root->wavetbl),
					  "midi.driver", G_OBJECT (widg));

    /* Connect widgets to FluidSynth properties */
    swamigui_control_glade_prop_connect (fluid_widg, G_OBJECT (swamigui_root->wavetbl));
  }

  gtk_widget_show (fluid_widg);

  return (fluid_widg);
}

/* Callback when audio driver combo box changes */
static void
fluid_synth_gui_audio_driver_changed (GtkComboBox *combo, gpointer user_data)
{
  GtkWidget *fluid_widg = GTK_WIDGET (user_data);
  GtkWidget *driverwidg;
  GtkWidget *vbox;
  char *driver, *widgname;
  const char **sptr;

  driver = gtk_combo_box_get_active_text (combo);	/* ++ alloc */

  vbox = swamigui_util_glade_lookup (fluid_widg, "VBoxAudioDriver");

  /* Remove existing child, if any */
  gtk_container_foreach (GTK_CONTAINER (vbox), (GtkCallback)gtk_object_destroy, NULL);

  /* See if the driver has a widget */
  for (sptr = audio_driver_widgets; *sptr; sptr++)
    if (strcmp (driver, *sptr) == 0) break;

  if (*sptr)
  {
    widgname = g_strconcat ("FluidSynth-Audio:", driver, NULL);	/* ++ alloc */
    driverwidg = swamigui_util_glade_create (widgname);
    g_free (widgname);	/* -- free */

    if (driverwidg)
    {
      swamigui_control_glade_prop_connect (driverwidg,
					   G_OBJECT (swamigui_root->wavetbl));
      gtk_box_pack_start (GTK_BOX (vbox), driverwidg, FALSE, FALSE, 0);
    }
  }
}

/* Callback when MIDI driver combo box changes */
static void
fluid_synth_gui_midi_driver_changed (GtkComboBox *combo, gpointer user_data)
{
  GtkWidget *fluid_widg = GTK_WIDGET (user_data);
  GtkWidget *driverwidg;
  GtkWidget *vbox;
  char *driver, *widgname;
  const char **sptr;

  driver = gtk_combo_box_get_active_text (combo);	/* ++ alloc */

  vbox = swamigui_util_glade_lookup (fluid_widg, "VBoxMidiDriver");

  /* Remove existing child, if any */
  gtk_container_foreach (GTK_CONTAINER (vbox), (GtkCallback)gtk_object_destroy, NULL);

  /* See if the driver has a widget */
  for (sptr = midi_driver_widgets; *sptr; sptr++)
    if (strcmp (driver, *sptr) == 0) break;

  if (*sptr)
  {
    widgname = g_strconcat ("FluidSynth-MIDI:", driver, NULL);	/* ++ alloc */
    driverwidg = swamigui_util_glade_create (widgname);
    g_free (widgname);	/* -- free */

    if (driverwidg)
    {
      swamigui_control_glade_prop_connect (driverwidg,
					   G_OBJECT (swamigui_root->wavetbl));
      gtk_box_pack_start (GTK_BOX (vbox), driverwidg, FALSE, FALSE, 0);
    }
  }
}

static GType
fluid_synth_gui_control_register_type (SwamiPlugin *plugin)
{
  static const GTypeInfo obj_info =
    {
      sizeof (FluidSynthGuiControlClass), NULL, NULL,
      (GClassInitFunc)fluid_synth_gui_control_class_init, NULL, NULL,
      sizeof (FluidSynthGuiControl), 0,
      (GInstanceInitFunc) fluid_synth_gui_control_init,
    };

  return (g_type_module_register_type (G_TYPE_MODULE (plugin),
				       GTK_TYPE_VBOX, "FluidSynthGuiControl",
				       &obj_info, 0));
}

static void
fluid_synth_gui_control_class_init (FluidSynthGuiControlClass *klass)
{
}

static void
fluid_synth_gui_control_init (FluidSynthGuiControl *fsctrl)
{
  SwamiWavetbl *wavetbl;
  char namebuf[32];
  const char *knobnames[] =
    { "Gain", "ReverbLevel", "ReverbRoom", "ReverbWidth", "ReverbDamp",
      "ChorusLevel", "ChorusCount", "ChorusFreq", "ChorusDepth" };
  const char *propnames[] =
    { "synth-gain", "reverb-level", "reverb-room-size", "reverb-width",
      "reverb-damp", "chorus-level", "chorus-count", "chorus-freq",
      "chorus-depth" };
  SwamiControl *propctrl, *widgctrl;
  GtkAdjustment *adj;
  GtkWidget *widg;
  GType type;
  int i;

  fsctrl->ctrl_widg = swamigui_util_glade_create ("FluidSynth");
  gtk_widget_show (fsctrl->ctrl_widg);
  gtk_box_pack_start (GTK_BOX (fsctrl), fsctrl->ctrl_widg, FALSE, FALSE, 0);

  wavetbl = (SwamiWavetbl *)swami_object_get_by_type (G_OBJECT (swamigui_root),
						      "WavetblFluidSynth");     // ++ ref wavetbl
  if (!wavetbl) return;

  for (i = 0; i < G_N_ELEMENTS (knobnames); i++)
  {
    strcpy (namebuf, "Knob");
    strcat (namebuf, knobnames[i]);
    widg = swamigui_util_glade_lookup (fsctrl->ctrl_widg, namebuf);
    adj = swamigui_knob_get_adjustment (SWAMIGUI_KNOB (widg));

    propctrl = swami_get_control_prop_by_name (G_OBJECT (wavetbl), propnames[i]);
    widgctrl = SWAMI_CONTROL (swamigui_control_adj_new (adj));

    swami_control_connect (propctrl, widgctrl, SWAMI_CONTROL_CONN_BIDIR
			   | SWAMI_CONTROL_CONN_INIT | SWAMI_CONTROL_CONN_SPEC);
  }

  propctrl = swami_get_control_prop_by_name (G_OBJECT (wavetbl), "synth.reverb.active");
  widg = swamigui_util_glade_lookup (fsctrl->ctrl_widg, "BtnReverb");
  widgctrl = swamigui_control_new_for_widget (G_OBJECT (widg));
  swami_control_connect (propctrl, widgctrl, SWAMI_CONTROL_CONN_BIDIR
			 | SWAMI_CONTROL_CONN_INIT | SWAMI_CONTROL_CONN_SPEC);

  propctrl = swami_get_control_prop_by_name (G_OBJECT (wavetbl), "synth.chorus.active");
  widg = swamigui_util_glade_lookup (fsctrl->ctrl_widg, "BtnChorus");
  widgctrl = swamigui_control_new_for_widget (G_OBJECT (widg));
  swami_control_connect (propctrl, widgctrl, SWAMI_CONTROL_CONN_BIDIR
			 | SWAMI_CONTROL_CONN_INIT | SWAMI_CONTROL_CONN_SPEC);

  widg = swamigui_util_glade_lookup (fsctrl->ctrl_widg, "ComboChorusType");
  propctrl = swami_get_control_prop_by_name (G_OBJECT (wavetbl), "chorus-waveform");
  type = g_type_from_name ("WavetblFluidSynthChorusWaveform");
  widgctrl = swamigui_control_new_for_widget_full (G_OBJECT (widg), type, NULL, 0);
  swami_control_connect (propctrl, widgctrl, SWAMI_CONTROL_CONN_BIDIR
			 | SWAMI_CONTROL_CONN_INIT | SWAMI_CONTROL_CONN_SPEC);

  g_object_unref (wavetbl);     // -- unref wavetbl
}

