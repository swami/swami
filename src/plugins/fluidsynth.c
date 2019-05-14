/*
 * fluidsynth.c - Swami plugin for FluidSynth
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
#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <libswami/libswami.h>
#include <swamigui/swamigui.h>
#include <libinstpatch/libinstpatch.h>

#include <fluidsynth.h>

#include "fluidsynth_i18n.h"

/* max voices per instrument (voices exceeding this will not sound) */
#define MAX_INST_VOICES   128

/* maximum # of voices under real time control (voices exceeding this number
   just wont be controllable in real time, no fatal problems though) */
#define MAX_REALTIME_VOICES    64

/* maximum realtime effect parameter updates for a single property change */
#define MAX_REALTIME_UPDATES   128

/* default number of synth channels (FIXME - dynamic?) */
#define DEFAULT_CHANNEL_COUNT 16

/* max length of reverb/chorus preset names (including '\0') */
#define PRESET_NAME_LEN 21


typedef struct _WavetblFluidSynth WavetblFluidSynth;
typedef struct _WavetblFluidSynthClass WavetblFluidSynthClass;

#define WAVETBL_TYPE_FLUIDSYNTH   (wavetbl_type)
#define WAVETBL_FLUIDSYNTH(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), WAVETBL_TYPE_FLUIDSYNTH, \
   WavetblFluidSynth))
#define WAVETBL_FLUIDSYNTH_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), WAVETBL_TYPE_FLUIDSYNTH, \
   WavetblFluidSynthClass))
#define WAVETBL_IS_FLUIDSYNTH(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), WAVETBL_TYPE_FLUIDSYNTH))
#define WAVETBL_IS_FLUIDSYNTH_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), WAVETBL_TYPE_FLUIDSYNTH))

typedef struct _realtime_noteon_t realtime_noteon_t;

#define INTERPOLATION_TYPE (interp_mode_type)

/* structure for storing reverb parameters */
typedef struct
{
  char name[PRESET_NAME_LEN];		/* for presets */
  double room_size;
  double damp;
  double width;
  double level;
} ReverbParams;

/* structure for storing chorus parameters */
typedef struct
{
  char name[PRESET_NAME_LEN];		/* for presets */
  int count;
  double level;
  double freq;
  double depth;
  int waveform;
} ChorusParams;

#define CHORUS_WAVEFORM_TYPE (chorus_waveform_type)


/* FluidSynth SwamiWavetbl object */
struct _WavetblFluidSynth
{
  SwamiWavetbl object;		/* derived from SwamiWavetbl */

  fluid_synth_t *synth;		/* the FluidSynth handle */
  fluid_settings_t *settings;	/* to free on close */
  fluid_audio_driver_t *audio;	/* FluidSynth audio driver */
  fluid_midi_driver_t *midi;	/* FluidSynth MIDI driver */
  fluid_midi_router_t *midi_router; /* FluidSynth MIDI router */

  SwamiControlMidi *midi_ctrl;	/* MIDI control */
  guint prop_callback_handler_id;	/* property change handler ID */
  GSList *mods;			/* session modulators */

  int channel_count;		/* number of MIDI channels */
  guint8 *banks;		/* bank numbers for each channel */
  guint8 *programs;		/* program numbers for each channel */

  int interp;			/* interpolation type */

  gboolean reverb_update;	/* TRUE if reverb needs update */
  ReverbParams reverb_params;	/* current reverb parameters */

  gboolean chorus_update;	/* TRUE if chorus needs update */
  ChorusParams chorus_params;	/* current chorus parameters */

  /* active item is the focus, allow realtime control of most recent note of active item. */
  IpatchItem *active_item;			/* active audible instrument */
  IpatchItem *solo_item;                        /* child of active item to solo or NULL */
  IpatchSF2VoiceCache *rt_cache;		/* voice cache of active_item */
  int rt_sel_values[IPATCH_SF2_VOICE_CACHE_MAX_SEL_VALUES]; /* sel criteria of note */
  fluid_voice_t *rt_voices[MAX_REALTIME_VOICES]; /* FluidSynth voices */
  int rt_count;			/* count for rt_index_array and rt_voices */
};

/* FluidSynth wavetbl class */
struct _WavetblFluidSynthClass
{
  SwamiWavetblClass parent_class; /* derived from SwamiWavetblClass */
};

enum {
  WTBL_PROP_0,
  WTBL_PROP_INTERP,
  WTBL_PROP_REVERB_PRESET,
  WTBL_PROP_REVERB_ROOM_SIZE,
  WTBL_PROP_REVERB_DAMP,
  WTBL_PROP_REVERB_WIDTH,
  WTBL_PROP_REVERB_LEVEL,
  WTBL_PROP_CHORUS_PRESET,
  WTBL_PROP_CHORUS_COUNT,
  WTBL_PROP_CHORUS_LEVEL,
  WTBL_PROP_CHORUS_FREQ,
  WTBL_PROP_CHORUS_DEPTH,
  WTBL_PROP_CHORUS_WAVEFORM,
  WTBL_PROP_ACTIVE_ITEM,
  WTBL_PROP_SOLO_ITEM,
  WTBL_PROP_MODULATORS
};

/* number to use for first dynamic (FluidSynth settings) property */
#define FIRST_DYNAMIC_PROP  256

/* FluidSynth MIDI event types (MIDI control codes) */
enum
{
  WAVETBL_FLUID_NOTE_OFF = 0x80,
  WAVETBL_FLUID_NOTE_ON = 0x90,
  WAVETBL_FLUID_CONTROL_CHANGE = 0xb0,
  WAVETBL_FLUID_PROGRAM_CHANGE = 0xc0,
  WAVETBL_FLUID_PITCH_BEND = 0xe0
};

#define _SYNTH_OK 0		/* FLUID_OK not defined in header */

/* additional data for sfloader patch base objects */
typedef struct
{
  WavetblFluidSynth *wavetbl;	/* wavetable object */
  IpatchBase *base_item;	/* IpatchBase object */
} sfloader_sfont_data_t;

typedef struct
{
  WavetblFluidSynth *wavetbl;	/* wavetable object */
  IpatchItem *item;		/* instrument item */
} sfloader_preset_data_t;

/* FluidSynth property flags (for exceptions such as string booleans) */
typedef enum
{
  PROP_STRING_BOOL = 1 << 0
} PropFlags;

static gboolean plugin_fluidsynth_init (SwamiPlugin *plugin, GError **err);
static gboolean plugin_fluidsynth_save_xml (SwamiPlugin *plugin, GNode *xmlnode,
                                            GError **err);
static gboolean plugin_fluidsynth_load_xml (SwamiPlugin *plugin, GNode *xmlnode,
                                            GError **err);
static GType wavetbl_register_type (SwamiPlugin *plugin);
static GType interp_mode_register_type (SwamiPlugin *plugin);
static GType chorus_waveform_register_type (SwamiPlugin *plugin);

static void wavetbl_fluidsynth_class_init (WavetblFluidSynthClass *klass);
static void settings_foreach_count (void *data, const char *name, int type);
static void settings_foreach_option_count (void *data, const char *name, const char *option);
static void settings_foreach_func (void *data, const char *name, int type);
static void settings_foreach_option_func (void *data, const char *name, const char *option);
static int cmpstringp (const void *p1, const void *p2);
static void wavetbl_fluidsynth_init (WavetblFluidSynth *wavetbl);
static void wavetbl_fluidsynth_finalize (GObject *object);
static void wavetbl_fluidsynth_set_property (GObject *object,
					     guint property_id,
					     const GValue *value,
					     GParamSpec *pspec);
static void wavetbl_fluidsynth_get_property (GObject *object,
					     guint property_id,
					     GValue *value, GParamSpec *pspec);
static void wavetbl_fluidsynth_dispatch_properties_changed (GObject *object,
							    guint n_pspecs,
							    GParamSpec **pspecs);
static void
wavetbl_fluidsynth_midi_ctrl_callback (SwamiControl *control,
				       SwamiControlEvent *event,
				       const GValue *value);
static gboolean wavetbl_fluidsynth_open (SwamiWavetbl *swami_wavetbl,
					 GError **err);
static void wavetbl_fluidsynth_prop_callback (IpatchItemPropNotify *notify);
static int wavetbl_fluidsynth_handle_midi_event (void* data,
						 fluid_midi_event_t* event);
static void wavetbl_fluidsynth_close (SwamiWavetbl *swami_wavetbl);
static SwamiControlMidi *
wavetbl_fluidsynth_get_control (SwamiWavetbl *swami_wavetbl, int index);
static gboolean wavetbl_fluidsynth_load_patch (SwamiWavetbl *swami_wavetbl,
					       IpatchItem *patch,
					       GError **err);
static gboolean wavetbl_fluidsynth_load_active_item (SwamiWavetbl *swami_wavetbl,
						     IpatchItem *item, GError **err);
static gboolean wavetbl_fluidsynth_check_update_item (SwamiWavetbl *wavetbl,
						      IpatchItem *item,
						      GParamSpec *prop);
static void wavetbl_fluidsynth_update_item (SwamiWavetbl *wavetbl,
					    IpatchItem *item);
static void wavetbl_fluidsynth_update_reverb (WavetblFluidSynth *wavetbl);
static int find_reverb_preset (const char *name);
static void wavetbl_fluidsynth_update_chorus (WavetblFluidSynth *wavetbl);
static int find_chorus_preset (const char *name);

static fluid_sfont_t *sfloader_load_sfont (fluid_sfloader_t *loader,
					   const char *filename);
static int sfloader_sfont_free (fluid_sfont_t *sfont);
static const char *sfloader_sfont_get_name (fluid_sfont_t *sfont);
static fluid_preset_t *sfloader_sfont_get_preset (fluid_sfont_t *sfont,
						  int bank,
						  int prenum);
static void sfloader_preset_free (fluid_preset_t *preset);
static void sfloader_active_preset_free (fluid_preset_t *preset);
static const char *sfloader_preset_get_name (fluid_preset_t *preset);
static const char *sfloader_active_preset_get_name (fluid_preset_t *preset);
static int sfloader_preset_get_banknum (fluid_preset_t *preset);
static int sfloader_active_preset_get_banknum (fluid_preset_t *preset);
static int sfloader_preset_get_num (fluid_preset_t *preset);
static int sfloader_active_preset_get_num (fluid_preset_t *preset);
static int sfloader_preset_noteon (fluid_preset_t *preset,
				   fluid_synth_t *synth,
				   int chan, int key, int vel);
static int sfloader_active_preset_noteon (fluid_preset_t *preset,
					fluid_synth_t *synth,
					int chan, int key, int vel);
static void cache_instrument (WavetblFluidSynth *wavetbl, IpatchItem *item);
static int cache_instrument_noteon (WavetblFluidSynth *wavetbl, IpatchItem *item,
				    fluid_synth_t *synth, int chan, int key,
				    int vel);
static void active_item_realtime_update (WavetblFluidSynth *wavetbl,
				         IpatchItem *item, GParamSpec *pspec,
				         const GValue *value);

/* FluidSynth settings boolean exceptions (yes/no string values) */
static const char *settings_str_bool[] = {
  "audio.jack.multi",
  "synth.chorus.active",
  "synth.dump",
  "synth.ladspa.active",
  "synth.reverb.active",
  "synth.verbose",
  NULL
};

/* types are defined as global vars */
static GType wavetbl_type = 0;
static GType interp_mode_type = 0;
static GType chorus_waveform_type = 0;

static GObjectClass *wavetbl_parent_class = NULL;

/* last dynamic property ID (incremented for each dynamically installed prop) */
static guint last_property_id = FIRST_DYNAMIC_PROP;

/* stores all dynamic FluidSynth setting names for mapping between property
   ID and FluidSynth setting */
static char **dynamic_prop_names;

/* stores PropFlags for property exceptions (string booleans, etc) */
static guint8 *dynamic_prop_flags;

/* Quark key used for assigning FluidSynth options string arrays to GParamSpecs */
GQuark wavetbl_fluidsynth_options_quark;

/* keeps a hash of patch objects to SF2VoiceCache objects */
G_LOCK_DEFINE_STATIC (voice_cache_hash);
static GHashTable *voice_cache_hash = NULL;

/* Reverb and Chorus preset tables (index 0 contains default values) */
G_LOCK_DEFINE_STATIC (preset_tables);	/* lock for reverb and chorus tables */

static ReverbParams *reverb_presets = NULL;
static ChorusParams *chorus_presets = NULL;
static int reverb_presets_count;	/* count of reverb presets */
static int chorus_presets_count;	/* count of chorus presets */

/* count of built in reverb and chorus presets */
#define REVERB_PRESETS_BUILTIN	1
#define CHORUS_PRESETS_BUILTIN	1

/* set plugin information */
SWAMI_PLUGIN_INFO (plugin_fluidsynth_init, NULL);

/* --- functions --- */


/* plugin init function (one time initialize of SwamiPlugin) */
static gboolean
plugin_fluidsynth_init (SwamiPlugin *plugin, GError **err)
{
    fluid_settings_t *settings = NULL;
    int idef;
    double ddef;
    
  /* bind the gettext domain */
#if defined(ENABLE_NLS)
  bindtextdomain ("SwamiPlugin-fluidsynth", LOCALEDIR);
#endif

  plugin->save_xml = plugin_fluidsynth_save_xml;
  plugin->load_xml = plugin_fluidsynth_load_xml;

  g_object_set (plugin,
		"name", "FluidSynth",
		"version", "1.1",
		"author", "Element Green",
		"copyright", "Copyright (C) 2002-2014",
		"descr", N_("FluidSynth software wavetable synth plugin"),
		"license", "GPL",
		NULL);

  
  /* initialize voice cache hash */
  voice_cache_hash = g_hash_table_new_full (NULL, NULL, NULL,
					    (GDestroyNotify)g_object_unref);
  /* initialize built-in reverb and chorus presets
   * !! Make sure that name field ends in NULLs (strncpy does this).
   * !! If not, then a potential multi-thread string crash could occur.
   */
  reverb_presets = g_new (ReverbParams, REVERB_PRESETS_BUILTIN);
  chorus_presets = g_new (ChorusParams, CHORUS_PRESETS_BUILTIN);
  settings = new_fluid_settings();
  
  if(voice_cache_hash == NULL || reverb_presets == NULL || chorus_presets == NULL || settings == NULL)
  {
      g_hash_table_unref(voice_cache_hash);
      voice_cache_hash = NULL;
      g_free(reverb_presets);
      reverb_presets = NULL;
      g_free(chorus_presets);
      chorus_presets = NULL;
      delete_fluid_settings(settings);
      return FALSE;
  }

  strncpy (reverb_presets[0].name, N_("Default"), PRESET_NAME_LEN);
  
  fluid_settings_getnum_default(settings, "synth.reverb.room-size", &ddef);
  reverb_presets[0].room_size = ddef;
  
  fluid_settings_getnum_default(settings, "synth.reverb.damp", &ddef);
  reverb_presets[0].damp = ddef;
  
  fluid_settings_getnum_default(settings, "synth.reverb.width", &ddef);
  reverb_presets[0].width = ddef;
  
  fluid_settings_getnum_default(settings, "synth.reverb.level", &ddef);
  reverb_presets[0].level = ddef;


  strncpy (chorus_presets[0].name, N_("Default"), PRESET_NAME_LEN);
  
  fluid_settings_getint_default(settings, "synth.chorus.nr", &idef);
  chorus_presets[0].count = idef;
  
  fluid_settings_getnum_default(settings, "synth.chorus.level", &ddef);
  chorus_presets[0].level = ddef;
  
  fluid_settings_getnum_default(settings, "synth.chorus.speed", &ddef);
  chorus_presets[0].freq = ddef;
  
  fluid_settings_getnum_default(settings, "synth.chorus.depth", &ddef);
  chorus_presets[0].depth = ddef;
  
  chorus_presets[0].waveform = FLUID_CHORUS_MOD_SINE;

  delete_fluid_settings(settings);
  
  
  /* initialize types */
  wavetbl_type = wavetbl_register_type (plugin);
  interp_mode_type = interp_mode_register_type (plugin);
  chorus_waveform_type = chorus_waveform_register_type (plugin);

  return (TRUE);
}

static gboolean
plugin_fluidsynth_save_xml (SwamiPlugin *plugin, GNode *xmlnode, GError **err)
{
  WavetblFluidSynth *wavetbl;
  
  /* get swamigui_root from swamigui library */
  SwamiguiRoot * swamigui_root = swamigui_get_swamigui_root ();

  if (!swamigui_root || !swamigui_root->wavetbl
      || !WAVETBL_IS_FLUIDSYNTH (swamigui_root->wavetbl))
  {
    g_set_error (err, SWAMI_ERROR, SWAMI_ERROR_FAIL,
                 "Failure saving FluidSynth preferences: No FluidSynth object");
    return (FALSE);
  }

  wavetbl = WAVETBL_FLUIDSYNTH (swamigui_root->wavetbl);

  return (ipatch_xml_encode_object (xmlnode, G_OBJECT (wavetbl), FALSE, err));
}

static gboolean
plugin_fluidsynth_load_xml (SwamiPlugin *plugin, GNode *xmlnode, GError **err)
{
  WavetblFluidSynth *wavetbl;

  /* get swamigui_root from swamigui library */
  SwamiguiRoot * swamigui_root = swamigui_get_swamigui_root ();
  if (!swamigui_root || !swamigui_root->wavetbl
      || !WAVETBL_IS_FLUIDSYNTH (swamigui_root->wavetbl))
  {
    g_set_error (err, SWAMI_ERROR, SWAMI_ERROR_FAIL,
                 "Failure loading FluidSynth preferences: No FluidSynth object");
    return (FALSE);
  }

  wavetbl = WAVETBL_FLUIDSYNTH (swamigui_root->wavetbl);

  return (ipatch_xml_decode_object (xmlnode, G_OBJECT (wavetbl), err));
}

static GType
wavetbl_register_type (SwamiPlugin *plugin)
{
  static const GTypeInfo obj_info =
    {
      sizeof (WavetblFluidSynthClass), NULL, NULL,
      (GClassInitFunc) wavetbl_fluidsynth_class_init, NULL, NULL,
      sizeof (WavetblFluidSynth), 0,
      (GInstanceInitFunc) wavetbl_fluidsynth_init,
    };

  return (g_type_module_register_type (G_TYPE_MODULE (plugin),
				       SWAMI_TYPE_WAVETBL, "WavetblFluidSynth",
				       &obj_info, 0));
}

static GType
interp_mode_register_type (SwamiPlugin *plugin)
{
  static const GEnumValue values[] =
    {
      { FLUID_INTERP_NONE,
	"WAVETBL_FLUIDSYNTH_INTERP_NONE", "None" },
      { FLUID_INTERP_LINEAR,
	"WAVETBL_FLUIDSYNTH_INTERP_LINEAR", "Linear" },
      { FLUID_INTERP_4THORDER,
	"WAVETBL_FLUIDSYNTH_INTERP_4THORDER", "4th Order" },
      { FLUID_INTERP_7THORDER,
	"WAVETBL_FLUIDSYNTH_INTERP_7THORDER", "7th Order" },
      { 0, NULL, NULL }
    };
  static GTypeInfo enum_info = { 0, NULL, NULL, NULL, NULL, NULL, 0, 0, NULL };

  /* initialize the type info structure for the enum type */
  g_enum_complete_type_info (G_TYPE_ENUM, &enum_info, values);

  return (g_type_module_register_type (G_TYPE_MODULE (plugin), G_TYPE_ENUM,
				       "WavetblFluidSynthInterpType",
				       &enum_info, 0));
}

static GType
chorus_waveform_register_type (SwamiPlugin *plugin)
{
  static const GEnumValue values[] =
    {
      { FLUID_CHORUS_MOD_SINE,
	"WAVETBL_FLUID_CHORUS_MOD_SINE", "Sine" },
      { FLUID_CHORUS_MOD_TRIANGLE,
	"WAVETBL_FLUID_CHORUS_MOD_TRIANGLE", "Triangle" },
      { 0, NULL, NULL }
    };
  static GTypeInfo enum_info = { 0, NULL, NULL, NULL, NULL, NULL, 0, 0, NULL };

  /* initialize the type info structure for the enum type */
  g_enum_complete_type_info (G_TYPE_ENUM, &enum_info, values);

  return (g_type_module_register_type (G_TYPE_MODULE (plugin), G_TYPE_ENUM,
				       "WavetblFluidSynthChorusWaveform",
				       &enum_info, 0));
}

/* used for passing multiple values to FluidSynth foreach function */
typedef struct
{
  fluid_settings_t *settings;
  GObjectClass *klass;
  int count;
} ForeachBag;

static void
wavetbl_fluidsynth_class_init (WavetblFluidSynthClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  SwamiWavetblClass *wavetbl_class;
  ForeachBag bag;

  wavetbl_parent_class = g_type_class_peek_parent (klass);
  obj_class->finalize = wavetbl_fluidsynth_finalize;
  obj_class->set_property = wavetbl_fluidsynth_set_property;
  obj_class->get_property = wavetbl_fluidsynth_get_property;
  obj_class->dispatch_properties_changed
    = wavetbl_fluidsynth_dispatch_properties_changed;

  wavetbl_class = SWAMI_WAVETBL_CLASS (klass);
  wavetbl_class->open = wavetbl_fluidsynth_open;
  wavetbl_class->close = wavetbl_fluidsynth_close;
  wavetbl_class->get_control = wavetbl_fluidsynth_get_control;
  wavetbl_class->load_patch = wavetbl_fluidsynth_load_patch;
  wavetbl_class->load_active_item = wavetbl_fluidsynth_load_active_item;
  wavetbl_class->check_update_item = wavetbl_fluidsynth_check_update_item;
  wavetbl_class->update_item = wavetbl_fluidsynth_update_item;

  wavetbl_fluidsynth_options_quark = g_quark_from_static_string ("FluidSynth-options");

  /* used for dynamically installing settings (required for settings queries) */
  bag.settings = new_fluid_settings ();
  bag.klass = obj_class;
  bag.count = 0;

  /* count the number of FluidSynth properties + options properties */
  fluid_settings_foreach (bag.settings, &bag, settings_foreach_count);

  /* have to keep a mapping of property IDs to FluidSynth setting names, since
     GObject properties get mangled ('.' turns to '-') */
  dynamic_prop_names = g_malloc (bag.count * sizeof (char *));

  /* allocate array for property exception flags */
  dynamic_prop_flags = g_malloc0 (bag.count * sizeof (guint8));


  /* add all FluidSynth settings as class properties */
  fluid_settings_foreach (bag.settings, &bag, settings_foreach_func);

  delete_fluid_settings (bag.settings);		/* not needed anymore */
  
  
  g_object_class_install_property (obj_class, WTBL_PROP_INTERP,
			g_param_spec_enum ("interp", _("Interpolation"),
					   _("Interpolation type"),
					   INTERPOLATION_TYPE,
					   FLUID_INTERP_DEFAULT,
					   G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, WTBL_PROP_REVERB_PRESET,
			g_param_spec_string ("reverb-preset", _("Reverb preset"),
					     _("Reverb preset"),
					     NULL, G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, WTBL_PROP_REVERB_ROOM_SIZE,
		g_param_spec_double ("reverb-room-size", _("Reverb room size"),
				     _("Reverb room size"),
				     0.0, 1.0, reverb_presets[0].room_size,
                                     G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, WTBL_PROP_REVERB_DAMP,
		g_param_spec_double ("reverb-damp", _("Reverb damp"),
				     _("Reverb damp"),
				     0.0, 1.0, reverb_presets[0].damp,
                                     G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, WTBL_PROP_REVERB_WIDTH,
			g_param_spec_double ("reverb-width", _("Reverb width"),
					     _("Reverb width"),
					     0.0, 100.0, reverb_presets[0].width,
					     G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, WTBL_PROP_REVERB_LEVEL,
			g_param_spec_double ("reverb-level", _("Reverb level"),
					     _("Reverb level"),
					     0.0, 1.0, reverb_presets[0].level,
					     G_PARAM_READWRITE));

  g_object_class_install_property (obj_class, WTBL_PROP_CHORUS_PRESET,
			g_param_spec_string ("chorus-preset", _("Chorus preset"),
					     _("Chorus preset"),
					     NULL, G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, WTBL_PROP_CHORUS_COUNT,
			g_param_spec_int ("chorus-count", _("Chorus count"),
					  _("Number of chorus delay lines"),
					  1, 99, chorus_presets[0].count,
                                          G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, WTBL_PROP_CHORUS_LEVEL,
		g_param_spec_double ("chorus-level", _("Chorus level"),
				     _("Output level of each chorus line"),
				     0.0, 10.0, chorus_presets[0].level,
                                     G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, WTBL_PROP_CHORUS_FREQ,
		g_param_spec_double ("chorus-freq", _("Chorus freq"),
				     _("Chorus modulation frequency (Hz)"),
				     0.3, 5.0, chorus_presets[0].freq,
                                     G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, WTBL_PROP_CHORUS_DEPTH,
			g_param_spec_double ("chorus-depth", _("Chorus depth"),
					     _("Chorus depth"),
					     0.0, 20.0, chorus_presets[0].depth,
					     G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, WTBL_PROP_CHORUS_WAVEFORM,
		g_param_spec_enum ("chorus-waveform", _("Chorus waveform"),
				   _("Chorus waveform type"),
				   CHORUS_WAVEFORM_TYPE,
				   FLUID_CHORUS_MOD_SINE,
				   G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, WTBL_PROP_ACTIVE_ITEM,
		g_param_spec_object ("active-item", _("Active item"),
				   _("Active focused audible item"),
				   IPATCH_TYPE_ITEM,
				   G_PARAM_READWRITE | IPATCH_PARAM_NO_SAVE));
  g_object_class_install_property (obj_class, WTBL_PROP_SOLO_ITEM,
		g_param_spec_object ("solo-item", _("Solo item"),
				   _("Child of active item to solo"),
				   IPATCH_TYPE_ITEM,
				   G_PARAM_READWRITE | IPATCH_PARAM_NO_SAVE));
  g_object_class_install_property (obj_class, WTBL_PROP_MODULATORS,
		g_param_spec_boxed ("modulators", _("Modulators"),
				   _("Modulators"),
				   IPATCH_TYPE_SF2_MOD_LIST,
				   G_PARAM_READWRITE | IPATCH_PARAM_NO_SAVE));
}

/* for counting the number of FluidSynth settings properties */
static void
settings_foreach_count (void *data, const char *name, int type)
{
  ForeachBag *bag = (ForeachBag *)data;
  int optcount = 0;

  bag->count = bag->count + 1;

  /* if there are string options, add one for an "-options" property */
  if (type == FLUID_STR_TYPE)
  {
    fluid_settings_foreach_option (bag->settings, name, &optcount,
				   settings_foreach_option_count);
    if (optcount > 0) bag->count = bag->count + 1;
  }
}

/* function to count FluidSynth string options for a parameter */
static void
settings_foreach_option_count (void *data, const char *name, const char *option)
{
  int *optcount = data;
  *optcount = *optcount + 1;
}

/* add each FluidSynth setting as a GObject property */
static void
settings_foreach_func (void *data, const char *name, int type)
{
  ForeachBag *bag = (ForeachBag *)data;
  GStrv options = NULL, optionp;
  GParamSpec *spec;
  double dmin, dmax, ddef;
  int imin, imax, idef, hint;
  gboolean bdef;
  char *defstr = NULL;
  const char **sp;
  int optcount = 0;
  char *optname;

  /* check if this property is on the string boolean list */
  for (sp = settings_str_bool; *sp; sp++)
    if (type == FLUID_STR_TYPE && strcmp (name, *sp) == 0) break;

  if (*sp)	/* string boolean value? */
  {
    bdef = fluid_settings_str_equal (bag->settings, name, "yes");
    spec = g_param_spec_boolean (name, name, name, bdef, G_PARAM_READWRITE);

    /* set PROP_STRING_BOOL property flag */
    dynamic_prop_flags[last_property_id - FIRST_DYNAMIC_PROP] |= PROP_STRING_BOOL;
  }
  else
  {
    switch (type)
    {
    case FLUID_NUM_TYPE:
      fluid_settings_getnum_range (bag->settings, name, &dmin, &dmax);
      fluid_settings_getnum_default (bag->settings, name, &ddef);
      spec = g_param_spec_double (name, name, name, dmin, dmax, ddef,
				  G_PARAM_READWRITE);
      break;
    case FLUID_INT_TYPE:
      fluid_settings_getint_range (bag->settings, name, &imin, &imax);
      fluid_settings_getint_default (bag->settings, name, &idef);
      fluid_settings_get_hints(bag->settings, name, &hint);

      if ((hint | FLUID_HINT_TOGGLED) != 0)	/* boolean parameter? */
	spec = g_param_spec_boolean (name, name, name, idef != 0, G_PARAM_READWRITE);
      else
	spec = g_param_spec_int (name, name, name, imin, imax, idef, G_PARAM_READWRITE);
      break;
    case FLUID_STR_TYPE:
      fluid_settings_getstr_default (bag->settings, name, &defstr);
      spec = g_param_spec_string (name, name, name, defstr, G_PARAM_READWRITE);

      /* count options for this string parameter (if any) */
      fluid_settings_foreach_option (bag->settings, name, &optcount,
				     settings_foreach_option_count);

      /* if there are any options, create a string array and assign them */
      if (optcount > 0)
      {
	options = g_new (char *, optcount + 1);	/* ++ alloc string array */
	optionp = options;
	fluid_settings_foreach_option (bag->settings, name, &optionp,
				       settings_foreach_option_func);
	options[optcount] = NULL;

	/* Sort the options alphabetically */
	qsort (options, optcount, sizeof (options[0]), cmpstringp);
      }
      break;
    case FLUID_SET_TYPE:
      g_warning ("Enum not handled for property '%s'", name);
      return;
    default:
      return;
    }
  }

  /* install the property */
  g_object_class_install_property (bag->klass, last_property_id, spec);

  /* store the name to the property name array */
  dynamic_prop_names[last_property_id - FIRST_DYNAMIC_PROP] = g_strdup (name);

  last_property_id++;	/* advance the last dynamic property ID */

  /* install an options parameter if there are any string options */
  if (options)
  {
    optname = g_strconcat (name, "-options", NULL);	/* ++ alloc */
    spec = g_param_spec_boxed (optname, optname, optname, G_TYPE_STRV,
                               G_PARAM_READABLE);

    /* !! GParamSpec takes over allocation of options array */
    g_param_spec_set_qdata (spec, wavetbl_fluidsynth_options_quark, options);
    g_object_class_install_property (bag->klass, last_property_id, spec);

    /* !! takes over allocation */
    dynamic_prop_names[last_property_id - FIRST_DYNAMIC_PROP] = optname;

    last_property_id++;	/* advance the last dynamic property ID */
  }
}

/* function to iterate over FluidSynth string options for string parameters
 * and fill a string array */
static void
settings_foreach_option_func (void *data, const char *name, const char *option)
{
  GStrv *poptions = data;

  **poptions = g_strdup (option);
  *poptions = *poptions + 1;
}

/* qsort function to sort array of option strings */
static int
cmpstringp (const void *p1, const void *p2)
{
  return strcmp (*(char * const *)p1, *(char * const *)p2);
}

static void
wavetbl_fluidsynth_init (WavetblFluidSynth *wavetbl)
{
  wavetbl->synth = NULL;
  wavetbl->settings = new_fluid_settings ();

  wavetbl->midi_ctrl = swami_control_midi_new (); /* ++ ref */
  swami_control_midi_set_callback (wavetbl->midi_ctrl,
				   wavetbl_fluidsynth_midi_ctrl_callback,
				   wavetbl);

  wavetbl->channel_count = DEFAULT_CHANNEL_COUNT;
  wavetbl->banks = g_new0 (guint8, wavetbl->channel_count);
  wavetbl->programs = g_new0 (guint8, wavetbl->channel_count);

  wavetbl->reverb_params = reverb_presets[0];
  wavetbl->chorus_params = chorus_presets[0];

  wavetbl->active_item = NULL;
  wavetbl->rt_cache = NULL;
  wavetbl->rt_count = 0;
}

static void
wavetbl_fluidsynth_finalize (GObject *object)
{
  WavetblFluidSynth *wavetbl = WAVETBL_FLUIDSYNTH (object);

  g_free (wavetbl->banks);
  g_free (wavetbl->programs);

  if (wavetbl->midi_ctrl) g_object_unref (wavetbl->midi_ctrl); /* -- unref */
  if (wavetbl->settings) delete_fluid_settings (wavetbl->settings);

  G_OBJECT_CLASS (wavetbl_parent_class)->finalize (object);
}

static void
wavetbl_fluidsynth_set_property (GObject *object, guint property_id,
				 const GValue *value, GParamSpec *pspec)
{
  WavetblFluidSynth *wavetbl = WAVETBL_FLUIDSYNTH (object);
  GSList *oldmods, *newmods;
  IpatchItem *item, *active_item;
  char *name;
  const char *s;
  int retval;
  int index;

  /* is it one of the dynamically installed properties? */
  if (property_id >= FIRST_DYNAMIC_PROP && property_id < last_property_id)
    {
      name = dynamic_prop_names[property_id - FIRST_DYNAMIC_PROP];

      switch (G_PARAM_SPEC_VALUE_TYPE (pspec))
	{
	case G_TYPE_INT:
	  retval = fluid_settings_setint (wavetbl->settings, name,
					  g_value_get_int (value));
	  break;
	case G_TYPE_DOUBLE:
	  retval = fluid_settings_setnum (wavetbl->settings, name,
					  g_value_get_double (value));
	  break;
	case G_TYPE_STRING:
	  retval = fluid_settings_setstr (wavetbl->settings, name,
					  (char *)g_value_get_string (value));
	  break;
	case G_TYPE_BOOLEAN:
	  /* check if its a string boolean property */
	  if (dynamic_prop_flags[property_id - FIRST_DYNAMIC_PROP] & PROP_STRING_BOOL)
	    retval = fluid_settings_setstr (wavetbl->settings, name,
					    g_value_get_boolean (value) ? "yes" : "no");
	  else retval = fluid_settings_setint (wavetbl->settings, name,
					       g_value_get_boolean (value));
	  break;
	default:
	  g_critical ("Unexpected FluidSynth dynamic property type");
	  return;
	}

      if (retval == FLUID_FAILED)
	g_critical ("Failed to set FluidSynth property '%s'", name);

      return;
    }

  switch (property_id)
    {
    case WTBL_PROP_INTERP:
      wavetbl->interp = g_value_get_enum (value);

      SWAMI_LOCK_WRITE (wavetbl);
      if (wavetbl->synth)
	fluid_synth_set_interp_method (wavetbl->synth, -1, wavetbl->interp);
      SWAMI_UNLOCK_WRITE (wavetbl);
      break;
    case WTBL_PROP_REVERB_PRESET:
      s = g_value_get_string (value);
      index = 0;

      if (s && s[0] != '\0')	/* valid preset name? */
      {
	G_LOCK (preset_tables);
	index = find_reverb_preset (s);
	if (index != 0) wavetbl->reverb_params = reverb_presets[index];
	G_UNLOCK (preset_tables);
      }

      if (index == 0) wavetbl->reverb_params = reverb_presets[0];
      wavetbl->reverb_update = TRUE;
      break;
    case WTBL_PROP_REVERB_ROOM_SIZE:
      wavetbl->reverb_params.name[0] = '\0';	/* clear preset name (if any) */
      wavetbl->reverb_params.room_size = g_value_get_double (value);
      wavetbl->reverb_update = TRUE;
      break;
    case WTBL_PROP_REVERB_DAMP:
      wavetbl->reverb_params.name[0] = '\0';	/* clear preset name (if any) */
      wavetbl->reverb_params.damp = g_value_get_double (value);
      wavetbl->reverb_update = TRUE;
      break;
    case WTBL_PROP_REVERB_WIDTH:
      wavetbl->reverb_params.name[0] = '\0';	/* clear preset name (if any) */
      wavetbl->reverb_params.width = g_value_get_double (value);
      wavetbl->reverb_update = TRUE;
      break;
    case WTBL_PROP_REVERB_LEVEL:
      wavetbl->reverb_params.name[0] = '\0';	/* clear preset name (if any) */
      wavetbl->reverb_params.level = g_value_get_double (value);
      wavetbl->reverb_update = TRUE;
      break;
    case WTBL_PROP_CHORUS_PRESET:
      s = g_value_get_string (value);
      index = 0;

      if (s && s[0] != '\0')	/* valid preset name? */
      {
	G_LOCK (preset_tables);
	index = find_chorus_preset (s);
	if (index != 0) wavetbl->chorus_params = chorus_presets[index];
	G_UNLOCK (preset_tables);
      }

      if (index == 0) wavetbl->chorus_params = chorus_presets[0];
      wavetbl->chorus_update = TRUE;
      break;
    case WTBL_PROP_CHORUS_COUNT:
      wavetbl->chorus_params.name[0] = '\0';	/* clear preset name (if any) */
      wavetbl->chorus_params.count = g_value_get_int (value);
      wavetbl->chorus_update = TRUE;
      break;
    case WTBL_PROP_CHORUS_LEVEL:
      wavetbl->chorus_params.name[0] = '\0';	/* clear preset name (if any) */
      wavetbl->chorus_params.level = g_value_get_double (value);
      wavetbl->chorus_update = TRUE;
      break;
    case WTBL_PROP_CHORUS_FREQ:
      wavetbl->chorus_params.name[0] = '\0';	/* clear preset name (if any) */
      wavetbl->chorus_params.freq = g_value_get_double (value);
      wavetbl->chorus_update = TRUE;
      break;
    case WTBL_PROP_CHORUS_DEPTH:
      wavetbl->chorus_params.name[0] = '\0';	/* clear preset name (if any) */
      wavetbl->chorus_params.depth = g_value_get_double (value);
      wavetbl->chorus_update = TRUE;
      break;
    case WTBL_PROP_CHORUS_WAVEFORM:
      wavetbl->chorus_params.name[0] = '\0';	/* clear preset name (if any) */
      wavetbl->chorus_params.waveform = g_value_get_enum (value);
      wavetbl->chorus_update = TRUE;
      break;
    case WTBL_PROP_ACTIVE_ITEM:
      item = g_value_get_object (value);

      SWAMI_LOCK_WRITE (wavetbl);
      wavetbl_fluidsynth_load_active_item ((SwamiWavetbl *)wavetbl, item, NULL);
      SWAMI_UNLOCK_WRITE (wavetbl);
      break;
    case WTBL_PROP_SOLO_ITEM:
      item = g_value_get_object (value);

      SWAMI_LOCK_WRITE (wavetbl);
      if (wavetbl->solo_item) g_object_unref (wavetbl->solo_item);
      wavetbl->solo_item = g_value_dup_object (value);
      active_item = g_object_ref (wavetbl->active_item);        /* ++ ref active item */
      SWAMI_UNLOCK_WRITE (wavetbl);

      wavetbl_fluidsynth_update_item ((SwamiWavetbl *)wavetbl, active_item);
      g_object_unref (active_item);     /* -- unref active item */
      break;
    case WTBL_PROP_MODULATORS:
      newmods = g_value_dup_boxed (value);

      SWAMI_LOCK_WRITE (wavetbl);
      oldmods = wavetbl->mods;
      wavetbl->mods = newmods;
      SWAMI_UNLOCK_WRITE (wavetbl);

      if (oldmods) ipatch_sf2_mod_list_free (oldmods, TRUE);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
wavetbl_fluidsynth_get_property (GObject *object, guint property_id,
				 GValue *value, GParamSpec *pspec)
{
  WavetblFluidSynth *wavetbl = WAVETBL_FLUIDSYNTH (object);
  GSList *mods;
  char s[256];
  char *name;
  double d;
  int retval;
  int i;
  GStrv strv;

  /* is it one of the dynamically installed properties? */
  if (property_id >= FIRST_DYNAMIC_PROP && property_id < last_property_id)
    {
      name = dynamic_prop_names[property_id - FIRST_DYNAMIC_PROP];

      switch (G_PARAM_SPEC_VALUE_TYPE (pspec))
	{
	case G_TYPE_INT:
	  retval = fluid_settings_getint (wavetbl->settings, name, &i);
	  if (retval != FLUID_FAILED) g_value_set_int (value, i);
	  break;
	case G_TYPE_DOUBLE:
	  retval = fluid_settings_getnum (wavetbl->settings, name, &d);
	  if (retval != FLUID_FAILED) g_value_set_double (value, d);
	  break;
	case G_TYPE_STRING:
	  retval = fluid_settings_copystr (wavetbl->settings, name, s, sizeof(s));
	  if (retval != FLUID_FAILED) g_value_set_string (value, s);
	  break;
	case G_TYPE_BOOLEAN:
	  /* check if its a string boolean property */
	  if (dynamic_prop_flags[property_id - FIRST_DYNAMIC_PROP] & PROP_STRING_BOOL)
	  {
	    i = fluid_settings_str_equal (wavetbl->settings, name, "yes");
	    g_value_set_boolean (value, i);
	    retval = FLUID_OK;
	  }
	  else
	  {
	    retval = fluid_settings_getint (wavetbl->settings, name, &i);
	    if (retval != FLUID_FAILED) g_value_set_boolean (value, i);
	  }
	  break;
	default:
	  /* For FluidSynth string -options parameters */
	  if (G_PARAM_SPEC_VALUE_TYPE (pspec) == G_TYPE_STRV)
	  {
	    strv = g_param_spec_get_qdata (pspec, wavetbl_fluidsynth_options_quark);
	    g_value_set_boxed (value, strv);
        retval = FLUID_OK;
          }
	  else
	  {
	    g_critical ("Unexpected FluidSynth dynamic property type");
	    return;
	  }
	}

      if (retval == FLUID_FAILED)
	  g_critical ("Failed to get FluidSynth property '%s'", name);

      return;
    }

  switch (property_id)
    {
    case WTBL_PROP_INTERP:
      g_value_set_enum (value, wavetbl->interp);
      break;
    case WTBL_PROP_REVERB_PRESET:
      /* don't need to lock since buffer is static and will always contain a
       * NULL terminator (a partially updated string could occur though). */
      g_value_set_string (value, wavetbl->reverb_params.name);
      break;
    case WTBL_PROP_REVERB_ROOM_SIZE:
      g_value_set_double (value, wavetbl->reverb_params.room_size);
      break;
    case WTBL_PROP_REVERB_DAMP:
      g_value_set_double (value, wavetbl->reverb_params.damp);
      break;
    case WTBL_PROP_REVERB_WIDTH:
      g_value_set_double (value, wavetbl->reverb_params.width);
      break;
    case WTBL_PROP_REVERB_LEVEL:
      g_value_set_double (value, wavetbl->reverb_params.level);
      break;
    case WTBL_PROP_CHORUS_PRESET:
      /* don't need to lock since buffer is static and will always contain a
       * NULL terminator (a partially updated string could occur though). */
      g_value_set_string (value, wavetbl->chorus_params.name);
      break;
    case WTBL_PROP_CHORUS_COUNT:
      g_value_set_int (value, wavetbl->chorus_params.count);
      break;
    case WTBL_PROP_CHORUS_LEVEL:
      g_value_set_double (value, wavetbl->chorus_params.level);
      break;
    case WTBL_PROP_CHORUS_FREQ:
      g_value_set_double (value, wavetbl->chorus_params.freq);
      break;
    case WTBL_PROP_CHORUS_DEPTH:
      g_value_set_double (value, wavetbl->chorus_params.depth);
      break;
    case WTBL_PROP_CHORUS_WAVEFORM:
      g_value_set_enum (value, wavetbl->chorus_params.waveform);
      break;
    case WTBL_PROP_ACTIVE_ITEM:
      SWAMI_LOCK_READ (wavetbl);
      g_value_set_object (value, wavetbl->active_item);
      SWAMI_UNLOCK_READ (wavetbl);
      break;
    case WTBL_PROP_SOLO_ITEM:
      SWAMI_LOCK_READ (wavetbl);
      g_value_set_object (value, wavetbl->solo_item);
      SWAMI_UNLOCK_READ (wavetbl);
      break;
    case WTBL_PROP_MODULATORS:
      SWAMI_LOCK_READ (wavetbl);
      mods = ipatch_sf2_mod_list_duplicate (wavetbl->mods);
      SWAMI_UNLOCK_READ (wavetbl);

      g_value_take_boxed (value, mods);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

/* used to group reverb and/or chorus property updates, when changing multiple
 * properties, to prevent excess calculation */
static void
wavetbl_fluidsynth_dispatch_properties_changed (GObject *object,
						guint n_pspecs,
						GParamSpec **pspecs)
{
  WavetblFluidSynth *wavetbl = WAVETBL_FLUIDSYNTH (object);

  if (wavetbl->reverb_update || wavetbl->chorus_update)
  {
    SWAMI_LOCK_WRITE (wavetbl);
    if (wavetbl->reverb_update) wavetbl_fluidsynth_update_reverb (wavetbl);
    if (wavetbl->chorus_update) wavetbl_fluidsynth_update_chorus (wavetbl);
    SWAMI_UNLOCK_WRITE (wavetbl);
  }

  G_OBJECT_CLASS (wavetbl_parent_class)->dispatch_properties_changed
    (object, n_pspecs, pspecs);
}

/* MIDI control callback */
static void
wavetbl_fluidsynth_midi_ctrl_callback (SwamiControl *control,
				       SwamiControlEvent *event,
				       const GValue *value)
{
  WavetblFluidSynth *wavetbl
    = WAVETBL_FLUIDSYNTH (((SwamiControlFunc *)control)->user_data);
  fluid_synth_t *synth;
  GArray *valarray = NULL;
  SwamiMidiEvent *midi;
  int i, count = 1;		/* default for single values */

  if (!wavetbl->synth) return;
  synth = wavetbl->synth;

  /* if its multiple values, fetch the value array */
  if (G_VALUE_TYPE (value) == G_TYPE_ARRAY)
    {
      valarray = g_value_get_boxed (value);
      count = valarray->len;
    }

  i = 0;
  while (i < count)
    {
      if (valarray) value = &g_array_index (valarray, GValue, i);

      if (G_VALUE_TYPE (value) == SWAMI_TYPE_MIDI_EVENT
	  && (midi = g_value_get_boxed (value)))
	{
	  switch (midi->type)
	    {
	    case SWAMI_MIDI_NOTE_ON:
	      fluid_synth_noteon (synth, midi->channel, midi->data.note.note,
				  midi->data.note.velocity);
	      break;
	    case SWAMI_MIDI_NOTE_OFF:
	      fluid_synth_noteoff (synth, midi->channel, midi->data.note.note);
	      break;
	    case SWAMI_MIDI_PITCH_BEND:	/* FluidSynth uses 0-16383 */
	      fluid_synth_pitch_bend (synth, midi->channel,
				      midi->data.control.value + 8192);
	      break;
	    case SWAMI_MIDI_CONTROL:
	      fluid_synth_cc (synth, midi->channel, midi->data.control.param,
			      midi->data.control.value);
	      break;
	    case SWAMI_MIDI_CONTROL14:
	      if (midi->data.control.param == SWAMI_MIDI_CC_BANK_MSB)
		{	/* update channel bank # */
		  if (midi->channel < wavetbl->channel_count)
		    wavetbl->banks[midi->channel] = midi->data.control.value;

		  fluid_synth_bank_select (synth, midi->channel,
					   midi->data.control.value);
		}
	      else
		fluid_synth_cc (synth, midi->channel, midi->data.control.param,
				midi->data.control.value);
	      break;
	    case SWAMI_MIDI_PROGRAM_CHANGE:
	      /* update channel program # */
	      if (midi->channel < wavetbl->channel_count)
		wavetbl->programs[midi->channel] = midi->data.control.value;

	      fluid_synth_program_change (synth, midi->channel,
					  midi->data.control.value);
	      break;
	    default:
	      break;
	    }
	}
      i++;
    }
}

/** init function for FluidSynth Swami wavetable driver */
static gboolean
wavetbl_fluidsynth_open (SwamiWavetbl *swami_wavetbl, GError **err)
{
  WavetblFluidSynth *wavetbl = WAVETBL_FLUIDSYNTH (swami_wavetbl);
  fluid_sfloader_t *loader;
  int i;

  SWAMI_LOCK_WRITE (wavetbl);

  if (swami_wavetbl->active)
  {
    SWAMI_UNLOCK_WRITE (wavetbl);
    return (TRUE);
  }

  /* create new FluidSynth */
  wavetbl->synth = new_fluid_synth (wavetbl->settings);
  if (!wavetbl->synth)
    {
      g_set_error (err, SWAMI_ERROR, SWAMI_ERROR_FAIL,
		   _("Failed to create FluidSynth context"));
      SWAMI_UNLOCK_WRITE (wavetbl);
      return (FALSE);
    }

  /* hook our sfloader */
  loader = new_fluid_sfloader(sfloader_load_sfont, delete_fluid_sfloader);
  if(loader == NULL)
  {
      g_set_error (err, SWAMI_ERROR, SWAMI_ERROR_FAIL,
		   _("Failed to create FluidSynth sfloader"));
      SWAMI_UNLOCK_WRITE (wavetbl);
      return (FALSE);
  }
  fluid_sfloader_set_data(loader, wavetbl);
  fluid_synth_add_sfloader (wavetbl->synth, loader);

  wavetbl->audio = new_fluid_audio_driver (wavetbl->settings, wavetbl->synth);

  /* Load dummy SoundFont to make active items work - sfloader_load_sfont */
  fluid_synth_sfload (wavetbl->synth, "!", FALSE);

  /* create MIDI router to send MIDI to FluidSynth */
  wavetbl->midi_router =
    new_fluid_midi_router (wavetbl->settings,
			   wavetbl_fluidsynth_handle_midi_event,
			   (void *)wavetbl);
  if (wavetbl->midi_router)
    {
      wavetbl->midi =
	new_fluid_midi_driver (wavetbl->settings,
			       fluid_midi_router_handle_midi_event,
			       (void *)(wavetbl->midi_router));
      if (!wavetbl->midi)
	g_warning (_("Failed to create FluidSynth MIDI input driver"));
    }
  else g_warning (_("Failed to create MIDI input router"));


  /* update reverb */
  wavetbl->reverb_update = TRUE;
  wavetbl_fluidsynth_update_reverb (wavetbl);

  /* update chorus */
  wavetbl->chorus_update = TRUE;
  wavetbl_fluidsynth_update_chorus (wavetbl);

  /* load active item if set */
  if (wavetbl->active_item)
    wavetbl_fluidsynth_load_active_item (swami_wavetbl, wavetbl->active_item, NULL);

  /* restore bank and program channel selections */
  for (i = 0; i < wavetbl->channel_count; i++)
    {
      fluid_synth_bank_select (wavetbl->synth, i, wavetbl->banks[i]);
      fluid_synth_program_change (wavetbl->synth, i, wavetbl->programs[i]);
    }

  /* monitor all property changes */
  wavetbl->prop_callback_handler_id =
    ipatch_item_prop_connect (NULL, NULL, wavetbl_fluidsynth_prop_callback,
			      NULL, wavetbl);

  swami_wavetbl->active = TRUE;
  SWAMI_UNLOCK_WRITE (wavetbl);

  return (TRUE);
}

/* called for every property change */
static void
wavetbl_fluidsynth_prop_callback (IpatchItemPropNotify *notify)
{
  WavetblFluidSynth *wavetbl = (WavetblFluidSynth *)(notify->user_data);

  /* quick check to see if property has SYNTH flag set */
  if (!(notify->pspec->flags & IPATCH_PARAM_SYNTH)) return;

  /* check if changed item is a dependent of active audible (for realtime fx) */
  SWAMI_LOCK_READ (wavetbl);

  if (notify->item == wavetbl->active_item
      && notify->pspec->flags & IPATCH_PARAM_SYNTH_REALTIME)
    active_item_realtime_update (wavetbl, notify->item, notify->pspec,
			         notify->new_value);

  SWAMI_UNLOCK_READ (wavetbl);

  /* see if property change affects any loaded instruments */
  if (wavetbl_fluidsynth_check_update_item ((SwamiWavetbl *)wavetbl,
					    notify->item, notify->pspec))
    wavetbl_fluidsynth_update_item ((SwamiWavetbl *)wavetbl, notify->item);
}

/* Called for each event received from the FluidSynth MIDI router */
static int
wavetbl_fluidsynth_handle_midi_event (void* data, fluid_midi_event_t* event)
{
  WavetblFluidSynth *wavetbl = WAVETBL_FLUIDSYNTH (data);
  int type = fluid_midi_event_get_type (event);
  int chan = fluid_midi_event_get_channel (event);
  int retval;

  retval = fluid_synth_handle_midi_event (wavetbl->synth, event);

  switch (type)
  {
  case WAVETBL_FLUID_NOTE_ON:
    swami_control_midi_transmit (wavetbl->midi_ctrl,
				 SWAMI_MIDI_NOTE_ON, chan,
				 fluid_midi_event_get_key (event),
				 fluid_midi_event_get_velocity (event));
    break;
  case WAVETBL_FLUID_NOTE_OFF:
    swami_control_midi_transmit (wavetbl->midi_ctrl,
				 SWAMI_MIDI_NOTE_OFF, chan,
				 fluid_midi_event_get_key (event),
				 fluid_midi_event_get_velocity (event));
    break;
  case WAVETBL_FLUID_CONTROL_CHANGE:
    /* update current wavetable bank? */
    if (fluid_midi_event_get_control (event) == SWAMI_MIDI_CC_BANK_MSB
	&& chan < wavetbl->channel_count)
      wavetbl->banks[chan] = fluid_midi_event_get_value (event);

    swami_control_midi_transmit (wavetbl->midi_ctrl,
				 SWAMI_MIDI_CONTROL, chan,
				 fluid_midi_event_get_control (event), 
				 fluid_midi_event_get_value (event));
    break;
  case WAVETBL_FLUID_PROGRAM_CHANGE:
    if (chan < wavetbl->channel_count)
      wavetbl->programs[chan] = fluid_midi_event_get_program (event);

    swami_control_midi_transmit (wavetbl->midi_ctrl,
				 SWAMI_MIDI_PROGRAM_CHANGE, chan,
				 fluid_midi_event_get_program (event), 0);
    break;
  case WAVETBL_FLUID_PITCH_BEND:	/* FluidSynth uses 0-16383 */
    swami_control_midi_transmit (wavetbl->midi_ctrl,
				 SWAMI_MIDI_PITCH_BEND, chan,
				 fluid_midi_event_get_pitch (event) - 8192, 0);
    break;
  }

  return retval;
}

/* close function for FluidSynth driver */
static void
wavetbl_fluidsynth_close (SwamiWavetbl *swami_wavetbl)
{
  WavetblFluidSynth *wavetbl = WAVETBL_FLUIDSYNTH (swami_wavetbl);

  SWAMI_LOCK_WRITE (wavetbl);

  if (!swami_wavetbl->active)
  {
    SWAMI_UNLOCK_WRITE (wavetbl);
    return;
  }

  /* remove our property change callback */
  ipatch_item_prop_disconnect (wavetbl->prop_callback_handler_id);

  if (wavetbl->midi) delete_fluid_midi_driver (wavetbl->midi);
  if (wavetbl->midi_router) delete_fluid_midi_router (wavetbl->midi_router);
  if (wavetbl->audio) delete_fluid_audio_driver (wavetbl->audio);
  if (wavetbl->synth) delete_fluid_synth (wavetbl->synth);
  if (wavetbl->rt_cache) g_object_unref (wavetbl->rt_cache);

  wavetbl->midi = NULL;
  wavetbl->midi_router = NULL;
  wavetbl->audio = NULL;
  wavetbl->synth = NULL;
  wavetbl->rt_cache = NULL;
  wavetbl->rt_count = 0;

  swami_wavetbl->active = FALSE;

  SWAMI_UNLOCK_WRITE (wavetbl);
}

/* get MIDI control method */
static SwamiControlMidi *
wavetbl_fluidsynth_get_control (SwamiWavetbl *swami_wavetbl, int index)
{
  WavetblFluidSynth *wavetbl = WAVETBL_FLUIDSYNTH (swami_wavetbl);
  if (index == 0) return (wavetbl->midi_ctrl);
  return (NULL);
}

/* patch load function for FluidSynth driver */
static gboolean
wavetbl_fluidsynth_load_patch (SwamiWavetbl *swami_wavetbl, IpatchItem *patch,
			       GError **err)
{
  WavetblFluidSynth *wavetbl = WAVETBL_FLUIDSYNTH (swami_wavetbl);
  char s[16];			/* enough space to store printf "&%p" */

  if (!IPATCH_IS_BASE (patch))
  {
    g_set_error (err, SWAMI_ERROR, SWAMI_ERROR_UNSUPPORTED,
		 _("Unsupported item type '%s' for FluidSynth patch load"),
		 G_OBJECT_TYPE_NAME (patch));
    return (FALSE);
  }

  SWAMI_LOCK_WRITE (wavetbl);

  /* make sure synth has been initialized */
  if (swami_log_if_fail (swami_wavetbl->active))
  {
    SWAMI_UNLOCK_WRITE (wavetbl);
    return (FALSE);
  }

  /* load patch by pointer (our FluidSynth sfloader plugin will use it) */
  g_strdup_printf (s, "&%p", (void *)patch);
  fluid_synth_sfload (wavetbl->synth, s, FALSE);

  SWAMI_UNLOCK_WRITE (wavetbl);

  return (TRUE);
}

/* active item load function for FluidSynth driver */
static gboolean
wavetbl_fluidsynth_load_active_item (SwamiWavetbl *swami_wavetbl,
				     IpatchItem *item, GError **err)
{
  WavetblFluidSynth *wavetbl = WAVETBL_FLUIDSYNTH (swami_wavetbl);

  /* only set as active item if its convertable to a SF2 voice cache */
  if (item && ipatch_find_converter (G_OBJECT_TYPE (item),
				     IPATCH_TYPE_SF2_VOICE_CACHE))
    {
      SWAMI_LOCK_WRITE (wavetbl);

      if (wavetbl->active_item)	/* remove reference to any current active item */
	g_object_unref (wavetbl->active_item);

      wavetbl->active_item = g_object_ref (item); /* ++ add reference to item */

      if (wavetbl->rt_cache)
      {
	g_object_unref (wavetbl->rt_cache);
	wavetbl->rt_cache = NULL;
      }

      wavetbl->rt_count = 0;

      cache_instrument (wavetbl, item);	/* cache the instrument voices */

      SWAMI_UNLOCK_WRITE (wavetbl);
    }

  return (TRUE);
}

/* SwamiWavetbl method to check if an item needs to update its synthesis cache */
static gboolean
wavetbl_fluidsynth_check_update_item (SwamiWavetbl *wavetbl, IpatchItem *item,
				      GParamSpec *prop)
{
  IpatchSF2VoiceCache *cache;

  /* if parameter doesn't have the SYNTH flag set, then no update needed */
  if (!(prop->flags & IPATCH_PARAM_SYNTH)) return (FALSE);

  /* check if item is cached */
  G_LOCK (voice_cache_hash);
  cache = g_hash_table_lookup (voice_cache_hash, item);
  G_UNLOCK (voice_cache_hash);

  return (cache != NULL);
}

/* SwamiWavetbl method to update an item's synthesis cache */
static void
wavetbl_fluidsynth_update_item (SwamiWavetbl *wavetbl, IpatchItem *item)
{
  SWAMI_LOCK_WRITE (wavetbl);
  cache_instrument (WAVETBL_FLUIDSYNTH (wavetbl), item);
  SWAMI_UNLOCK_WRITE (wavetbl);
}

static void
wavetbl_fluidsynth_update_reverb (WavetblFluidSynth *wavetbl)
{
  g_return_if_fail (WAVETBL_IS_FLUIDSYNTH (wavetbl));
  if (!wavetbl->synth || !wavetbl->reverb_update) return;

  wavetbl->reverb_update = FALSE;

  fluid_synth_set_reverb (wavetbl->synth,
			  wavetbl->reverb_params.room_size,
			  wavetbl->reverb_params.damp,
			  wavetbl->reverb_params.width,
			  wavetbl->reverb_params.level);
}

/* Lock preset_tables before calling this function */
static int
find_reverb_preset (const char *name)
{
  int i;

  for (i = 0; i < reverb_presets_count; i++)
  {
    if (strcmp (reverb_presets[i].name, name) == 0)
      return (i);
  }

  return (0);
}

static void
wavetbl_fluidsynth_update_chorus (WavetblFluidSynth *wavetbl)
{
  g_return_if_fail (WAVETBL_IS_FLUIDSYNTH (wavetbl));
  if (!wavetbl->synth || !wavetbl->chorus_update) return;

  wavetbl->chorus_update = FALSE;

  fluid_synth_set_chorus (wavetbl->synth,
			  wavetbl->chorus_params.count,
			  wavetbl->chorus_params.level,
			  wavetbl->chorus_params.freq,
			  wavetbl->chorus_params.depth,
			  wavetbl->chorus_params.waveform);
}

/* Lock preset_tables before calling this function */
static int
find_chorus_preset (const char *name)
{
  int i;

  for (i = 0; i < chorus_presets_count; i++)
  {
    if (strcmp (chorus_presets[i].name, name) == 0)
      return (i);
  }

  return (0);
}


/* FluidSynth sfloader functions */


/** FluidSynth sfloader "load" function */
static fluid_sfont_t *
sfloader_load_sfont (fluid_sfloader_t *loader, const char *filename)
{
  fluid_sfont_t *sfont;
  sfloader_sfont_data_t *sfont_data;
  IpatchItem *item = NULL;

  /* file name should be a string in the printf form "&%p" where the
     pointer is a pointer to a IpatchBase object, or "!" for
     dummy SoundFont to get active preset item to work when no
     SoundFont banks loaded */
  if (filename[0] == '&')
    {
      sscanf (filename, "&%p", (void **)(&item));
      if (!item) return (NULL);
      g_object_ref (item);	/* ++ Add a reference to the patch object */
    }
  else if (filename[0] != '!')
    return (NULL);		/* didn't begin with '&' or '!' */

  sfont_data = g_malloc0 (sizeof (sfloader_sfont_data_t));
  sfont_data->wavetbl = (WavetblFluidSynth *)(fluid_sfloader_get_data(loader));
  sfont_data->base_item = IPATCH_BASE (item);

  sfont = new_fluid_sfont(sfloader_sfont_get_name, sfloader_sfont_get_preset, NULL, NULL, sfloader_sfont_free);
  fluid_sfont_set_data(sfont, sfont_data);
  
  return (sfont);
}

/* sfloader callback to clean up an fluid_sfont_t structure */
static int
sfloader_sfont_free (fluid_sfont_t *sfont)
{
  sfloader_sfont_data_t *sfont_data;

  sfont_data = (sfloader_sfont_data_t *)(fluid_sfont_get_data(sfont));

  if (sfont_data->base_item)	/* -- remove reference */
    g_object_unref (IPATCH_ITEM (sfont_data->base_item));

  g_free (sfont_data);
  delete_fluid_sfont (sfont);

  return (_SYNTH_OK);
}

/* sfloader callback to get a patch file name */
static const char *
sfloader_sfont_get_name (fluid_sfont_t *sfont)
{
  sfloader_sfont_data_t *sfont_data;
  static char buf[256];	/* using static buffer so info string can be freed */
  char *s;

  sfont_data = (sfloader_sfont_data_t *)(fluid_sfont_get_data(sfont));

  if (sfont_data->base_item)
    {
      g_object_get (sfont_data->base_item, "file-name", &s, NULL);
      g_strlcpy (buf, s, sizeof (buf));
      g_free (s);
    }
  else buf[0] = '\0';

  return (buf);
}

/* sfloader callback to get a preset (instrument) by bank and preset number */
static fluid_preset_t *
sfloader_sfont_get_preset (fluid_sfont_t *sfont, int bank,
			   int prenum)
{
  sfloader_sfont_data_t *sfont_data;
  sfloader_preset_data_t *preset_data;
  fluid_preset_t* preset;
  int b, p;

  sfont_data = (sfloader_sfont_data_t *)(fluid_sfont_get_data(sfont));

  /* active item bank:preset requested? */
  swami_wavetbl_get_active_item_locale (SWAMI_WAVETBL (sfont_data->wavetbl), &b, &p);

  if (bank == b && prenum == p)
    {
      g_object_ref (G_OBJECT (sfont_data->wavetbl)); /* ++ inc wavetbl ref */

      preset = new_fluid_preset(sfont,
                                sfloader_active_preset_get_name,
                                sfloader_active_preset_get_banknum,
                                sfloader_active_preset_get_num,
                                sfloader_active_preset_noteon,
                                sfloader_active_preset_free);
      fluid_preset_set_data(preset, sfont_data->wavetbl);
    }
  else				/* regular preset request */
    {
      IpatchItem *item;

      if (!sfont_data->base_item)  /* for active preset SoundFont HACK */
	return (NULL);

      /* ++ ref found MIDI instrument object */
      item = ipatch_base_find_item_by_midi_locale (sfont_data->base_item,
						       bank, prenum);
      if (!item) return (NULL);

      preset_data = g_malloc0 (sizeof (sfloader_preset_data_t));

      g_object_ref (G_OBJECT (sfont_data->wavetbl)); /* ++ inc wavetbl ref */
      preset_data->wavetbl = sfont_data->wavetbl;

      preset_data->item = item; /* !! item already referenced by find */

      preset = new_fluid_preset(sfont,
                                sfloader_preset_get_name,
                                sfloader_preset_get_banknum,
                                sfloader_preset_get_num,
                                sfloader_preset_noteon,
                                sfloader_preset_free);
      fluid_preset_set_data(preset, preset_data);
    }

  return (preset);
}

/* sfloader callback to clean up an fluid_preset_t structure */
static void 
sfloader_preset_free (fluid_preset_t *preset)
{
  sfloader_preset_data_t *preset_data;

  preset_data = fluid_preset_get_data(preset);

  /* -- remove item reference */
  g_object_unref (IPATCH_ITEM (preset_data->item));

  /* remove wavetable object reference */
  g_object_unref (G_OBJECT (preset_data->wavetbl));

  g_free (preset_data);
  delete_fluid_preset (preset);
}

/* sfloader callback to clean up a active item preset structure */
static void
sfloader_active_preset_free (fluid_preset_t *preset)
{
  g_object_unref (G_OBJECT (fluid_preset_get_data(preset))); /* -- remove wavetbl obj ref */
  delete_fluid_preset (preset);
}

/* sfloader callback to get the name of a preset */
static const char *
sfloader_preset_get_name (fluid_preset_t *preset)
{
  sfloader_preset_data_t *preset_data = fluid_preset_get_data(preset);
  static char buf[256]; /* return string is static */
  char *name;

  g_object_get (preset_data->item, "name", &name, NULL);
  g_strlcpy (buf, name, sizeof (buf));
  g_free (name);

  return (buf);
}

/* sfloader callback to get name of active preset */
static const char *
sfloader_active_preset_get_name (fluid_preset_t *preset)
{
  return (_("<active>"));
}

/* sfloader callback to get the bank number of a preset */
static int
sfloader_preset_get_banknum (fluid_preset_t *preset)
{
  sfloader_preset_data_t *preset_data = fluid_preset_get_data(preset);
  int bank;

  g_object_get (preset_data->item, "bank", &bank, NULL);
  return (bank);
}

/* sfloader callback to get the bank number of active preset */
static int
sfloader_active_preset_get_banknum (fluid_preset_t *preset)
{
  sfloader_preset_data_t *preset_data = fluid_preset_get_data(preset);
  int bank;

  g_object_get (preset_data->wavetbl, "active-bank", &bank, NULL);
  return (bank);
}

/* sfloader callback to get the preset number of a preset */
static int
sfloader_preset_get_num (fluid_preset_t *preset)
{
  sfloader_preset_data_t *preset_data = fluid_preset_get_data(preset);
  int program;

  g_object_get (preset_data->item, "program", &program, NULL);
  return (program);
}

/* sfloader callback to get the preset number of active preset */
static int
sfloader_active_preset_get_num (fluid_preset_t *preset)
{
  sfloader_preset_data_t *preset_data = fluid_preset_get_data(preset);
  int psetnum;

  g_object_get (preset_data->wavetbl, "active-program", &psetnum, NULL);
  return (psetnum);
}

/* sfloader callback for a noteon event */
static int
sfloader_preset_noteon (fluid_preset_t *preset, fluid_synth_t *synth,
			int chan, int key, int vel)
{
  sfloader_preset_data_t *preset_data = fluid_preset_get_data(preset);
  WavetblFluidSynth *wavetbl = fluid_preset_get_data(preset);

  /* No item matches the bank:program? */
  if (!preset_data->item) return (_SYNTH_OK);

  SWAMI_LOCK_WRITE (wavetbl);
  cache_instrument_noteon (wavetbl, preset_data->item, synth, chan, key, vel);
  SWAMI_UNLOCK_WRITE (wavetbl);

  return (_SYNTH_OK);
}

/* handles noteon event for active item */
static int
sfloader_active_preset_noteon (fluid_preset_t *preset, fluid_synth_t *synth,
			       int chan, int key, int vel)
{
  WavetblFluidSynth *wavetbl = fluid_preset_get_data(preset);

  SWAMI_LOCK_WRITE (wavetbl);
  if (!wavetbl->active_item)
  {
    SWAMI_UNLOCK_WRITE (wavetbl);
    return (_SYNTH_OK);	/* no active item? Do nothing.. */
  }

  cache_instrument_noteon (wavetbl, wavetbl->active_item, synth, chan, key, vel);
  SWAMI_UNLOCK_WRITE (wavetbl);

  return (_SYNTH_OK);
}

/* caches an instrument item into SoundFont voices for faster processing at
 * note-on time in cache_instrument_noteon().
 * MT-NOTE: Caller is responsible for wavetbl object locking.
 */
static void
cache_instrument (WavetblFluidSynth *wavetbl, IpatchItem *item)
{
  IpatchConverter *conv;
  IpatchSF2Voice *voice;
  IpatchSF2VoiceCache *cache;
  IpatchItem *solo_item = NULL;
  int i, count;

  /* ++ ref - create SF2 voice cache converter */
  conv = ipatch_create_converter (G_OBJECT_TYPE (item), IPATCH_TYPE_SF2_VOICE_CACHE);

  /* no SF2 voice cache converter for this item type? */
  if (!conv) return;

  SWAMI_LOCK_READ (wavetbl);
  if (wavetbl->solo_item)
    solo_item = g_object_ref (wavetbl->solo_item);        /* ++ ref solo item */
  SWAMI_UNLOCK_READ (wavetbl);

  g_object_set (conv, "solo-item", solo_item, NULL);

  cache = ipatch_sf2_voice_cache_new (NULL, 0);         /* ++ ref voice cache */

  /* copy session modulators to voice cache */
  cache->override_mods = ipatch_sf2_mod_list_duplicate (wavetbl->mods);

  ipatch_converter_add_input (conv, G_OBJECT (item));
  ipatch_converter_add_output (conv, G_OBJECT (cache));

  /* Convert item to SF2 voice cache and assign solo-item (if any)
   * ++ ref list */
  if (!ipatch_converter_convert (conv, NULL))
  {
    g_object_unref (cache);     /* -- unref voice cache object */
    if (solo_item) g_object_unref (solo_item);  /* -- unref solo item */
    g_object_unref (conv);      /* -- unref converter */
    return;
  }

  if (solo_item) g_object_unref (solo_item);  /* -- unref solo item */
  g_object_unref (conv);        /* -- unref converter */

  /* Use voice->user_data to close open cached stores */
  cache->voice_user_data_destroy = (GDestroyNotify)ipatch_sample_store_cache_close;

  /* loop over voices and load sample data into RAM */
  count = cache->voices->len;
  for (i = 0; i < count; i++)
    {
      voice = &g_array_index (cache->voices, IpatchSF2Voice, i);
      ipatch_sf2_voice_cache_sample_data (voice, NULL);

      /* Keep sample store cached by doing a dummy open */
      ipatch_sample_store_cache_open ((IpatchSampleStoreCache *)voice->sample_store);
      voice->user_data = voice->sample_store;
    }

  /* !! hash takes over voice cache reference */
  G_LOCK (voice_cache_hash);
  g_hash_table_insert (voice_cache_hash, item, cache);
  G_UNLOCK (voice_cache_hash);
}

/* noteon event function for cached instruments.
 * MT-NOTE: Caller is responsible for wavetbl object locking.
 */
static int
cache_instrument_noteon (WavetblFluidSynth *wavetbl, IpatchItem *item,
			 fluid_synth_t *synth, int chan, int key, int vel)
{
  guint16 index_array[MAX_INST_VOICES];		/* voice index array */
  int sel_values[IPATCH_SF2_VOICE_CACHE_MAX_SEL_VALUES];
  fluid_voice_t *fluid_voices[MAX_REALTIME_VOICES];
  IpatchSF2VoiceCache *cache;
  IpatchSF2VoiceSelInfo *sel_info;
  IpatchSF2GenArray *gen_array;
  fluid_voice_t *flvoice;
  fluid_sample_t *wusample;
  fluid_mod_t *wumod;
  IpatchSF2Mod *mod;
  IpatchSF2Voice *voice;
  int i, voice_count, voice_num;
  GSList *p;

  G_LOCK (voice_cache_hash);
  cache = g_hash_table_lookup (voice_cache_hash, item);
  if (cache) g_object_ref (cache);	/* ++ ref cache object */
  G_UNLOCK (voice_cache_hash);

  if (!cache) return (_SYNTH_OK);	/* instrument not yet cached? */

  for (i = 0; i < cache->sel_count; i++)
    {
      sel_info = &cache->sel_info[i];
      switch (sel_info->type)
	{
	case IPATCH_SF2_VOICE_SEL_NOTE:
	  sel_values[i] = key;
	  break;
	case IPATCH_SF2_VOICE_SEL_VELOCITY:
	  sel_values[i] = vel;
	  break;
	default:
	  sel_values[i] = 127;	/* FIXME */
	  break;
	}
    }

  voice_count = ipatch_sf2_voice_cache_select (cache, sel_values, index_array,
					       MAX_INST_VOICES);
  /* loop over matching voice indexes */
  for (voice_num = 0; voice_num < voice_count; voice_num++)
    {
      voice = IPATCH_SF2_VOICE_CACHE_GET_VOICE (cache, index_array[voice_num]);
      if (!voice->sample_store) continue;	/* For ROM and other non-readable samples */

      /* FIXME - pool of wusamples? */
      wusample = new_fluid_sample();

      fluid_sample_set_sound_data(wusample,
                                  ipatch_sample_store_cache_get_location((IpatchSampleStoreCache *)(voice->sample_store)),
                                  NULL,
                                  voice->sample_size,
                                  voice->rate,
                                  FALSE
                                 );
      
      fluid_sample_set_loop(wusample, voice->loop_start, voice->loop_end);
      fluid_sample_set_pitch(wusample, voice->root_note, voice->fine_tune);
      
      /* allocate the FluidSynth voice */
      flvoice = fluid_synth_alloc_voice (synth, wusample, chan, key, vel);
      if (!flvoice)
	{
        delete_fluid_sample (wusample);
	  g_object_unref (cache);	/* -- unref cache */
	  return (TRUE);
	}

      /* set only those generator parameters that are set */
      gen_array = &voice->gen_array;
      for (i = 0; i < IPATCH_SF2_GEN_COUNT; i++)
	if (IPATCH_SF2_GEN_ARRAY_TEST_FLAG (gen_array, i))
	  fluid_voice_gen_set (flvoice, i, (float)(gen_array->values[i].sword));

	  /* set modulators in fvoice internal list */
	  /* Note: here modulators are assumed non-linked modulators */
      wumod = g_alloca(fluid_mod_sizeof());
	  /* clear  fields is more safe.( in particular next field ) */
	  memset(wumod,0,fluid_mod_sizeof());
      p = voice->mod_list;
      while (p)
	{
	  mod = (IpatchSF2Mod *)(p->data);

    fluid_mod_set_dest(wumod, mod->dest);
    fluid_mod_set_source1(wumod,
                          mod->src & IPATCH_SF2_MOD_MASK_CONTROL,
                          ((mod->src & (IPATCH_SF2_MOD_MASK_DIRECTION
				       | IPATCH_SF2_MOD_MASK_POLARITY
				       | IPATCH_SF2_MOD_MASK_TYPE))
			  >> IPATCH_SF2_MOD_SHIFT_DIRECTION)
	    | ((mod->src & IPATCH_SF2_MOD_MASK_CC) ? FLUID_MOD_CC : 0));

	  fluid_mod_set_source2(wumod,
                          mod->amtsrc & IPATCH_SF2_MOD_MASK_CONTROL,
                          ((mod->amtsrc & (IPATCH_SF2_MOD_MASK_DIRECTION
					  | IPATCH_SF2_MOD_MASK_POLARITY
					  | IPATCH_SF2_MOD_MASK_TYPE))
			  >> IPATCH_SF2_MOD_SHIFT_DIRECTION)
	    | ((mod->amtsrc & IPATCH_SF2_MOD_MASK_CC) ? FLUID_MOD_CC : 0));

	  fluid_mod_set_amount(wumod, mod->amount);

	  fluid_voice_add_mod (flvoice, wumod, FLUID_VOICE_OVERWRITE);

	  p = p->next;
	}

      fluid_synth_start_voice (synth, flvoice); /* let 'er rip */

      /* voice pointers are only used for realtime note on, but not much CPU */
      if (voice_num < MAX_REALTIME_VOICES)
	fluid_voices[voice_num] = flvoice;

      /* !! store reference taken over by wusample structure */
    }

  g_object_unref (cache);	/* -- unref cache */

  /* check if item is the active audible, and update realtime vars if so */
  if (item == wavetbl->active_item)
  {
    if (wavetbl->rt_cache) g_object_unref (wavetbl->rt_cache);
    wavetbl->rt_cache = g_object_ref (cache);

    /* store selection criteria and FluidSynth voices for note event */
    memcpy (wavetbl->rt_sel_values, sel_values,
	    cache->sel_count * sizeof (sel_values[0]));
    memcpy (wavetbl->rt_voices, fluid_voices,
	    MIN (voice_count, MAX_REALTIME_VOICES) * sizeof (fluid_voices[0]));
    wavetbl->rt_count = voice_count;
  }

  return (_SYNTH_OK);
}

/* perform a realtime update on the active audible.
 * MT-NOTE: Wavetbl instance must be locked by caller.
 */
static void
active_item_realtime_update (WavetblFluidSynth *wavetbl, IpatchItem *item,
			     GParamSpec *pspec, const GValue *value)
{
  IpatchSF2VoiceUpdate updates[MAX_REALTIME_UPDATES], *upd;
  int count, i, rt_count;

  rt_count = wavetbl->rt_count;
  if (!wavetbl->rt_cache || rt_count == 0) return;

  count = ipatch_sf2_voice_cache_update
    (wavetbl->rt_cache, wavetbl->rt_sel_values, (GObject *)(wavetbl->active_item),
     (GObject *)item, pspec, value, updates, MAX_REALTIME_UPDATES);

  /* loop over updates and apply to FluidSynth voices */
  for (i = 0; i < count; i++)
  {
    upd = &updates[i];

    if (upd->voice < rt_count)
      fluid_voice_gen_set (wavetbl->rt_voices[upd->voice], upd->genid,
			   upd->ival);
  }

  /* update parameters (do separately so things are "more" atomic) */
  for (i = 0; i < count; i++)
  {
    upd = &updates[i];

    if (upd->voice < rt_count)
      fluid_voice_update_param (wavetbl->rt_voices[upd->voice], upd->genid);
  }
}
