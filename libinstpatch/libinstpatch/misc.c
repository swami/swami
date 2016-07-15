/*
 * libInstPatch
 * Copyright (C) 1999-2014 Element Green <element@elementsofsound.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; version 2.1
 * of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA or on the web at http://www.gnu.org.
 */
/**
 * SECTION: misc
 * @short_description: Miscellaneous stuff
 * @see_also: 
 * @stability: Stable
 */
#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <glib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

/* for mkdir */
#include <sys/stat.h>
#include <sys/types.h>

#include "libinstpatch.h"
#include "ipatch_priv.h"
#include "i18n.h"

/* private initializers in other source files */
void _ipatch_sf2_gen_init (void); /* IpatchSF2Gen.c */
void _ipatch_param_init (void);	/* IpatchParam.c */
void _ipatch_type_prop_init (void); /* IpatchTypeProp.c */
void _ipatch_util_init (void);	/* util.c */
void _ipatch_unit_init (void);	/* unit.c */
void _ipatch_xml_object_init (void);	/* IpatchXmlObject.c */
void _ipatch_range_init (void);	/* IpatchRange.c */

void _ipatch_convert_SF2_init (void);
void _ipatch_convert_gig_init (void);
void _ipatch_convert_DLS2_init (void);
void _ipatch_convert_SLI_init (void);
void _ipatch_sf2_voice_cache_init_DLS (void);
void _ipatch_sf2_voice_cache_init_SF2 (void);
void _ipatch_sf2_voice_cache_init_SLI (void);
void _ipatch_sf2_voice_cache_init_gig (void);
void _ipatch_sf2_voice_cache_init_VBank (void);

static gboolean ipatch_strv_xml_encode (GNode *node, GObject *object,
                                        GParamSpec *pspec, GValue *value,
                                        GError **err);
static gboolean ipatch_strv_xml_decode (GNode *node, GObject *object,
                                        GParamSpec *pspec, GValue *value,
                                        GError **err);
static void virtual_parent_dls2_inst (GType type, GParamSpec *spec,
				      GValue *value, GObject *object);
static void virtual_parent_gig_inst (GType type, GParamSpec *spec,
				     GValue *value, GObject *object);
static void virtual_parent_sf2_preset (GType type, GParamSpec *spec,
				       GValue *value, GObject *object);
static void virtual_parent_sf2_sample (GType type, GParamSpec *spec,
				       GValue *value, GObject *object);
static void conform_percussion (GObject *object);
static void conform_melodic (GObject *object);

static void dump_recursive (GObject *object, char *indent, FILE *file);
static void dump_object_info (GObject *object, char *indent, FILE *file);

typedef struct
{
  char *type_name;
  char *name;
  char *blurb;
  int category;
} TypePropInit;

/* info to initialize type properties */
static TypePropInit type_props[] =
{
  { "IpatchSampleStoreSndFile", N_("Sample file"), NULL, IPATCH_CATEGORY_SAMPLE },
  { "IpatchDLS2", N_("DLS"), N_("Down Loadable Sounds"), IPATCH_CATEGORY_BASE },
  { "IpatchDLS2Inst", N_("Instrument"), N_("DLS Instrument"), IPATCH_CATEGORY_PROGRAM },
  { "IpatchDLS2Region", N_("Region"), N_("DLS Region"), IPATCH_CATEGORY_SAMPLE_REF },
  { "IpatchDLS2Sample", N_("Sample"), N_("DLS Sample"), IPATCH_CATEGORY_SAMPLE },
  { "IpatchGig", N_("GigaSampler"), NULL, IPATCH_CATEGORY_BASE },
  { "IpatchGigDimension", N_("Dimension"), N_("GigaSampler Dimension"), IPATCH_CATEGORY_NONE },
  { "IpatchGigInst", N_("Instrument"), N_("GigaSampler Instrument"), IPATCH_CATEGORY_PROGRAM },
  { "IpatchGigRegion", N_("Region"), N_("GigaSampler Region"), IPATCH_CATEGORY_NONE },
  { "IpatchGigSample", N_("Sample"), N_("GigaSampler Sample"), IPATCH_CATEGORY_SAMPLE },
  { "IpatchGigSubRegion", N_("Sub Region"), N_("GigaSampler Sub Region"), IPATCH_CATEGORY_SAMPLE_REF },
  { "IpatchSF2", N_("SoundFont"), NULL, IPATCH_CATEGORY_BASE },
  { "IpatchSF2Inst", N_("Instrument"), N_("SoundFont Instrument"), IPATCH_CATEGORY_INSTRUMENT },
  { "IpatchSF2IZone", N_("Zone"), N_("SoundFont Instrument Zone"), IPATCH_CATEGORY_SAMPLE_REF },
  { "IpatchSF2Preset", N_("Preset"), N_("SoundFont Preset"), IPATCH_CATEGORY_PROGRAM },
  { "IpatchSF2PZone", N_("Zone"), N_("SoundFont Preset Zone"), IPATCH_CATEGORY_INSTRUMENT_REF },
  { "IpatchSF2Sample", N_("Sample"), N_("SoundFont Sample"), IPATCH_CATEGORY_SAMPLE },
  { "IpatchSLI", N_("Spectralis"), NULL, IPATCH_CATEGORY_BASE },
  { "IpatchSLIInst", N_("Instrument"), N_("Spectralis Instrument"), IPATCH_CATEGORY_INSTRUMENT },
  { "IpatchSLIZone", N_("Zone"), N_("Spectralis Instrument Zone"), IPATCH_CATEGORY_SAMPLE_REF },
  { "IpatchSLISample", N_("Sample"), N_("Spectralis Sample"), IPATCH_CATEGORY_SAMPLE },
  { "IpatchVBank", N_("VBank"), N_("Virtual Bank"), IPATCH_CATEGORY_BASE },
  { "IpatchVBankInst", N_("Instrument"), N_("VBank Instrument"), IPATCH_CATEGORY_PROGRAM },
  { "IpatchVBankRegion", N_("Region"), N_("VBank Region"), IPATCH_CATEGORY_INSTRUMENT_REF }
};

/* name of application using libInstPatch (for saving to files) */
char *ipatch_application_name = NULL;


/**
 * ipatch_init:
 *
 * Initialize libInstPatch library. Should be called before any other
 * libInstPatch related functions.
 */
void
ipatch_init (void)
{
  static gboolean initialized = FALSE;
  TypePropInit *prop_info;
  GType type;
  int i;

  if (initialized) return;
  initialized = TRUE;

  g_type_init ();

  if (!g_thread_supported ())
    g_thread_init (NULL);

  /* bind the gettext domain */
#if defined(ENABLE_NLS)
  bindtextdomain (PACKAGE, LOCALEDIR);
#endif

  /* Must be done before other types since they may be dependent */
  _ipatch_param_init ();
  _ipatch_type_prop_init ();
  _ipatch_unit_init ();
  _ipatch_xml_object_init ();
  _ipatch_util_init ();
  _ipatch_sf2_gen_init ();

  /* initialize interfaces before objects */
  ipatch_sample_get_type ();
  ipatch_sf2_gen_item_get_type ();
  ipatch_sf2_mod_item_get_type ();

  /* declares property types which other types may use */
  g_type_class_ref (IPATCH_TYPE_SF2_VOICE_CACHE);


  g_type_class_ref (IPATCH_TYPE_BASE);
  g_type_class_ref (IPATCH_TYPE_CONTAINER);
  g_type_class_ref (IPATCH_TYPE_CONVERTER);
  g_type_class_ref (IPATCH_TYPE_DLS2);
  ipatch_dls2_conn_get_type ();
  g_type_class_ref (IPATCH_TYPE_DLS2_INST);
  g_type_class_ref (IPATCH_TYPE_DLS2_REGION);
  g_type_class_ref (IPATCH_TYPE_DLS2_SAMPLE);
  g_type_class_ref (IPATCH_TYPE_DLS_FILE);
  g_type_class_ref (IPATCH_TYPE_DLS_READER);
  g_type_class_ref (IPATCH_TYPE_DLS_WRITER);
  g_type_class_ref (IPATCH_TYPE_FILE);
  ipatch_file_handle_get_type ();
  g_type_class_ref (IPATCH_TYPE_GIG_FILE);
  g_type_class_ref (IPATCH_TYPE_GIG);
  g_type_class_ref (IPATCH_TYPE_GIG_DIMENSION);
  g_type_class_ref (IPATCH_TYPE_GIG_INST);
  g_type_class_ref (IPATCH_TYPE_GIG_REGION);
  g_type_class_ref (IPATCH_TYPE_GIG_SAMPLE);
  g_type_class_ref (IPATCH_TYPE_GIG_SUB_REGION);
  g_type_class_ref (IPATCH_TYPE_ITEM);
  ipatch_iter_get_type ();
  g_type_class_ref (IPATCH_TYPE_LIST);
  ipatch_param_spec_range_get_type ();
  g_type_class_ref (IPATCH_TYPE_PASTE);
  ipatch_range_get_type ();
  g_type_class_ref (IPATCH_TYPE_RIFF);
  g_type_class_ref (IPATCH_TYPE_SAMPLE_DATA);
  g_type_class_ref (IPATCH_TYPE_SAMPLE_STORE);
  g_type_class_ref (IPATCH_TYPE_SAMPLE_STORE_FILE);
  g_type_class_ref (IPATCH_TYPE_SAMPLE_STORE_RAM);
  g_type_class_ref (IPATCH_TYPE_SAMPLE_STORE_ROM);
  g_type_class_ref (IPATCH_TYPE_SAMPLE_STORE_SND_FILE);
  g_type_class_ref (IPATCH_TYPE_SAMPLE_STORE_SPLIT24);
  g_type_class_ref (IPATCH_TYPE_SAMPLE_STORE_SWAP);
  g_type_class_ref (IPATCH_TYPE_SAMPLE_STORE_VIRTUAL);
  g_type_class_ref (IPATCH_TYPE_SF2_FILE);
  ipatch_sf2_gen_array_get_type ();
  g_type_class_ref (IPATCH_TYPE_SF2);
  g_type_class_ref (IPATCH_TYPE_SF2_INST);
  g_type_class_ref (IPATCH_TYPE_SF2_IZONE);
  g_type_class_ref (IPATCH_TYPE_SF2_READER);
  ipatch_sf2_mod_get_type ();
  ipatch_sf2_mod_list_get_type ();
  ipatch_sample_transform_get_type ();
  ipatch_sample_list_get_type ();
  ipatch_sample_list_item_get_type ();
  g_type_class_ref (IPATCH_TYPE_SF2_PRESET);
  g_type_class_ref (IPATCH_TYPE_SF2_PZONE);
  g_type_class_ref (IPATCH_TYPE_SF2_SAMPLE);
  g_type_class_ref (IPATCH_TYPE_SLI_FILE);
  g_type_class_ref (IPATCH_TYPE_SLI);
  g_type_class_ref (IPATCH_TYPE_SLI_INST);
  g_type_class_ref (IPATCH_TYPE_SLI_ZONE);
  g_type_class_ref (IPATCH_TYPE_SLI_SAMPLE);
  g_type_class_ref (IPATCH_TYPE_SLI_READER);
  g_type_class_ref (IPATCH_TYPE_VBANK);
  g_type_class_ref (IPATCH_TYPE_VBANK_INST);
  g_type_class_ref (IPATCH_TYPE_VBANK_REGION);
  g_type_class_ref (IPATCH_TYPE_SF2_WRITER);
  g_type_class_ref (IPATCH_TYPE_SF2_ZONE);
  g_type_class_ref (IPATCH_TYPE_SND_FILE);

  _ipatch_convert_SF2_init ();
  _ipatch_convert_gig_init ();
  _ipatch_convert_DLS2_init ();
  _ipatch_convert_SLI_init ();

  _ipatch_sf2_voice_cache_init_DLS ();
  _ipatch_sf2_voice_cache_init_SF2 ();
  _ipatch_sf2_voice_cache_init_SLI ();
  _ipatch_sf2_voice_cache_init_gig ();
  _ipatch_sf2_voice_cache_init_VBank ();

  _ipatch_range_init ();

  /* Register XML encode/decode handlers */

  /* GLib string array boxed type encode/decode */
  ipatch_xml_register_handler (G_TYPE_STRV, NULL, ipatch_strv_xml_encode,
                               ipatch_strv_xml_decode);

  /* set type properties */

  for (i = 0; i < G_N_ELEMENTS (type_props); i++)
    {
      type = g_type_from_name (type_props[i].type_name);
      if (log_if_fail (type != 0)) continue;

      prop_info = &type_props[i];

      if (prop_info->name)
	ipatch_type_set (type, "name", prop_info->name, NULL);

      if (prop_info->blurb)
	ipatch_type_set (type, "blurb", prop_info->blurb, NULL);

      if (prop_info->category != IPATCH_CATEGORY_NONE)
	ipatch_type_set (type, "category", prop_info->category, NULL);
    }

  /* link types */

  ipatch_type_set (IPATCH_TYPE_DLS2_REGION, "link-type",
		   IPATCH_TYPE_DLS2_SAMPLE, NULL);

  ipatch_type_set (IPATCH_TYPE_GIG_SUB_REGION, "link-type",
		   IPATCH_TYPE_GIG_SAMPLE, NULL);

  ipatch_type_set (IPATCH_TYPE_SF2_PZONE, "link-type",
		   IPATCH_TYPE_SF2_INST, NULL);

  ipatch_type_set (IPATCH_TYPE_SF2_IZONE, "link-type",
		   IPATCH_TYPE_SF2_SAMPLE, NULL);

  ipatch_type_set (IPATCH_TYPE_SLI_ZONE, "link-type",
		   IPATCH_TYPE_SLI_SAMPLE, NULL);

  ipatch_type_set (IPATCH_TYPE_VBANK_REGION, "link-type",
		   IPATCH_TYPE_ITEM, NULL);

  /* virtual container parent type properties */

  ipatch_type_set (IPATCH_TYPE_DLS2_SAMPLE,
		   "virtual-parent-type", IPATCH_TYPE_VIRTUAL_DLS2_SAMPLES,
		   NULL);
  ipatch_type_set (IPATCH_TYPE_GIG_SAMPLE,
		   "virtual-parent-type", IPATCH_TYPE_VIRTUAL_GIG_SAMPLES,
		   NULL);
  ipatch_type_set (IPATCH_TYPE_SF2_INST,
		   "virtual-parent-type", IPATCH_TYPE_VIRTUAL_SF2_INST,
		   NULL);
  ipatch_type_set (IPATCH_TYPE_SLI_INST,
		   "virtual-parent-type", IPATCH_TYPE_VIRTUAL_SLI_INST,
		   NULL);
  ipatch_type_set (IPATCH_TYPE_SLI_SAMPLE,
		   "virtual-parent-type", IPATCH_TYPE_VIRTUAL_SLI_SAMPLES,
		   NULL);

  /* dynamic virtual container properties (determined by object instance) */
  ipatch_type_set_dynamic_func (IPATCH_TYPE_DLS2_INST, "virtual-parent-type",
			     virtual_parent_dls2_inst);
  ipatch_type_set_dynamic_func (IPATCH_TYPE_GIG_INST, "virtual-parent-type",
			     virtual_parent_gig_inst);
  ipatch_type_set_dynamic_func (IPATCH_TYPE_SF2_PRESET, "virtual-parent-type",
			     virtual_parent_sf2_preset);
  ipatch_type_set_dynamic_func (IPATCH_TYPE_SF2_SAMPLE, "virtual-parent-type",
			     virtual_parent_sf2_sample);

  /* child object conform functions (for making a child object conform to a
   * specific virtual container) */
  ipatch_type_set (IPATCH_TYPE_VIRTUAL_DLS2_PERCUSSION,
		   "virtual-child-conform-func", conform_percussion,
		   NULL);
  ipatch_type_set (IPATCH_TYPE_VIRTUAL_DLS2_MELODIC,
		   "virtual-child-conform-func", conform_melodic,
		   NULL);
  ipatch_type_set (IPATCH_TYPE_VIRTUAL_GIG_PERCUSSION,
		   "virtual-child-conform-func", conform_percussion,
		   NULL);
  ipatch_type_set (IPATCH_TYPE_VIRTUAL_GIG_MELODIC,
		   "virtual-child-conform-func", conform_melodic,
		   NULL);
  ipatch_type_set (IPATCH_TYPE_VIRTUAL_SF2_PERCUSSION,
		   "virtual-child-conform-func", conform_percussion,
		   NULL);
  ipatch_type_set (IPATCH_TYPE_VIRTUAL_SF2_MELODIC,
		   "virtual-child-conform-func", conform_melodic,
		   NULL);

  /* container child sorting */
  ipatch_type_set (IPATCH_TYPE_VIRTUAL_DLS2_MELODIC,
		   "sort-children", TRUE, NULL);
  ipatch_type_set (IPATCH_TYPE_VIRTUAL_DLS2_PERCUSSION,
		   "sort-children", TRUE, NULL);

  ipatch_type_set (IPATCH_TYPE_VIRTUAL_GIG_MELODIC,
		   "sort-children", TRUE, NULL);
  ipatch_type_set (IPATCH_TYPE_VIRTUAL_GIG_PERCUSSION,
		   "sort-children", TRUE, NULL);

  ipatch_type_set (IPATCH_TYPE_VIRTUAL_SF2_MELODIC,
		   "sort-children", TRUE, NULL);
  ipatch_type_set (IPATCH_TYPE_VIRTUAL_SF2_PERCUSSION,
		   "sort-children", TRUE, NULL);

  ipatch_type_set (IPATCH_TYPE_VBANK,
		   "sort-children", TRUE, NULL);

  /* set "splits-type" properties */
  ipatch_type_set (IPATCH_TYPE_SF2_PRESET,
		   "splits-type", IPATCH_SPLITS_NORMAL, NULL);
  ipatch_type_set (IPATCH_TYPE_SF2_INST,
		   "splits-type", IPATCH_SPLITS_NORMAL, NULL);
  ipatch_type_set (IPATCH_TYPE_DLS2_INST,
		   "splits-type", IPATCH_SPLITS_NORMAL, NULL);
  ipatch_type_set (IPATCH_TYPE_GIG_INST,
		   "splits-type", IPATCH_SPLITS_NO_OVERLAP, NULL);
  ipatch_type_set (IPATCH_TYPE_SLI_INST,
		   "splits-type", IPATCH_SPLITS_NORMAL, NULL);
  ipatch_type_set (IPATCH_TYPE_VBANK_INST,
		   "splits-type", IPATCH_SPLITS_NORMAL, NULL);

  /* set "mime-type" properties */
  ipatch_type_set (IPATCH_TYPE_SF2_FILE,
                   "mime-type", "audio/x-soundfont", NULL);
  ipatch_type_set (IPATCH_TYPE_DLS_FILE,
                   "mime-type", "audio/dls", NULL);
  ipatch_type_set (IPATCH_TYPE_GIG_FILE,
                   "mime-type", "audio/x-gigasampler", NULL);
  ipatch_type_set (IPATCH_TYPE_SLI_FILE,
                   "mime-type", "audio/x-spectralis", NULL);
}

/**
 * ipatch_close:
 *
 * Perform cleanup of libInstPatch prior to application close.  Such as deleting
 * temporary files.
 *
 * Since: 1.1.0
 */
void
ipatch_close (void)
{
  ipatch_sample_store_swap_close ();
}

static gboolean
ipatch_strv_xml_encode (GNode *node, GObject *object, GParamSpec *pspec,
                        GValue *value, GError **err)
{
  GStrv strv;

  g_return_val_if_fail (G_VALUE_HOLDS (value, G_TYPE_STRV), FALSE);

  strv = g_value_get_boxed (value);

  if (!strv)
  {
    ipatch_xml_set_attribute (node, "null", "1");
    return (TRUE);
  }

  for (; *strv; strv++)
    ipatch_xml_new_node (node, "value", *strv, NULL);

  return (TRUE);
}

static gboolean
ipatch_strv_xml_decode (GNode *node, GObject *object, GParamSpec *pspec,
                        GValue *value, GError **err)
{
  GStrv strv;
  GNode *n;
  int i;

  g_return_val_if_fail (G_VALUE_HOLDS (value, G_TYPE_STRV), FALSE);

  if (ipatch_xml_test_attribute (node, "null", "1"))
  {
    g_value_set_boxed (value, NULL);
    return (TRUE);
  }

  /* Count "value" child nodes */
  for (i = 0, n = node->children; n; n = n->next)
    if (ipatch_xml_test_name (n, "value")) i++;

  strv = g_new (char *, i + 1);		/* ++ alloc new strv array */

  for (i = 0, n = node->children; n; n = n->next)
  {
    if (!ipatch_xml_test_name (n, "value")) continue;

    strv[i] = ipatch_xml_dup_value (n);
    i++;
  }

  strv[i] = NULL;

  g_value_take_boxed (value, strv);

  return (TRUE);
}

static void
virtual_parent_dls2_inst (GType type, GParamSpec *spec, GValue *value,
			  GObject *object)
{
  gboolean percuss = FALSE;

  if (object)
    g_object_get (object, "percussion", &percuss, NULL);

  if (percuss) g_value_set_gtype (value, IPATCH_TYPE_VIRTUAL_DLS2_PERCUSSION);
  else g_value_set_gtype (value, IPATCH_TYPE_VIRTUAL_DLS2_MELODIC);
}

static void
virtual_parent_gig_inst (GType type, GParamSpec *spec, GValue *value,
			 GObject *object)
{
  gboolean percuss = FALSE;

  if (object)
    g_object_get (object, "percussion", &percuss, NULL);

  if (percuss) g_value_set_gtype (value, IPATCH_TYPE_VIRTUAL_GIG_PERCUSSION);
  else g_value_set_gtype (value, IPATCH_TYPE_VIRTUAL_GIG_MELODIC);
}

static void
virtual_parent_sf2_preset (GType type, GParamSpec *spec, GValue *value,
			   GObject *object)
{
  gboolean percuss = FALSE;

  if (object)
    g_object_get (object, "percussion", &percuss, NULL);

  if (percuss) g_value_set_gtype (value, IPATCH_TYPE_VIRTUAL_SF2_PERCUSSION);
  else g_value_set_gtype (value, IPATCH_TYPE_VIRTUAL_SF2_MELODIC);
}

static void
virtual_parent_sf2_sample (GType type, GParamSpec *spec, GValue *value,
			   GObject *object)
{
  gboolean rom = FALSE;

  if (object) g_object_get (object, "rom", &rom, NULL);

  if (rom) g_value_set_gtype (value, IPATCH_TYPE_VIRTUAL_SF2_ROM);
  else g_value_set_gtype (value, IPATCH_TYPE_VIRTUAL_SF2_SAMPLES);
}

static void 
conform_percussion (GObject *object)
{
  g_object_set (object, "percussion", TRUE, NULL);
}

static void 
conform_melodic (GObject *object)
{
  g_object_set (object, "percussion", FALSE, NULL);
}


/**
 * ipatch_set_application_name:
 * @name: Application name and version (example: "swami 1.0") or %NULL to
 *   unset application name
 *
 * Set the global application name string which is used as the
 * software string written to patch files. This string should contain
 * the name of the application, and its version, that is using
 * libInstPatch. The libInstPatch version will also be output where
 * appropriate, so the software string written to a SoundFont for
 * example would look something like "swami 1.0 (libInstPatch 1.0)".
 */
void
ipatch_set_application_name (const char *name)
{
  if (ipatch_application_name) g_free (ipatch_application_name);

  if (name) ipatch_application_name = g_strdup (name);
  else ipatch_application_name = NULL;
}

/**
 * ipatch_version:
 * @major: (out) (optional): Pointer to store major version or %NULL
 * @minor: (out) (optional): Pointer to store minor version or %NULL
 * @micro: (out) (optional): Pointer to store micro version or %NULL
 *
 * Fetch the runtime version of the libInstPatch library.
 */
void
ipatch_version (guint *major, guint *minor, guint *micro)
{
  if (major) *major = IPATCH_VERSION_MAJOR;
  if (minor) *minor = IPATCH_VERSION_MINOR;
  if (micro) *micro = IPATCH_VERSION_MICRO;
}

GQuark
ipatch_error_quark (void)
{
  static GQuark q = 0;

  if (q == 0)
    q = g_quark_from_static_string ("libInstPatch-error-quark");

  return (q);
}

/**
 * _ret_g_log: (skip)
 */
int
_ret_g_log (const gchar *log_domain, GLogLevelFlags log_level,
	    const gchar *format, ...)
{
  va_list args;
  va_start (args, format);
  g_logv (log_domain, log_level, format, args);
  va_end (args);

  return (TRUE);
}

/**
 * ipatch_gerror_message: (skip)
 * @err:  A GError object or %NULL
 *
 * A utility function to check if a GError is set and return the
 * GError's message field if it is, or a string explaining that there
 * isn't any error info if @err is %NULL.
 *
 * Returns: The GError's message or a "&lt;No detailed error information&gt;" string.
 */
G_CONST_RETURN char *
ipatch_gerror_message (GError *err)
{
  return ((err) ? (err)->message : _("<No detailed error information>"));
}

/**
 * _ipatch_code_error: (skip)
 *
 * Internal function used by ipatch_code_error macros
 */
void
_ipatch_code_error (const char *file, guint line, const char *func,
		    GError **err, const char *format, ...)
{
  va_list args;
  va_start (args, format);
  _ipatch_code_errorv (file, line, func, err, format, args);
  va_end (args);
}

/**
 * _ipatch_code_errorv: (skip)
 *
 * Internal function used by ipatch_code_error macros
 */
void
_ipatch_code_errorv (const char *file, guint line, const char *func,
		     GError **err, const char *format, va_list args)
{
  char *msg, *loc, *temp;

  if (file && func) loc = g_strdup_printf ("%s:%d:%s()", file, line, func);
  else if (file) loc = g_strdup_printf ("%s:%d", file, line);
  else loc = NULL;

  temp = g_strdup_vprintf (format, args);
  msg = g_strdup_printf ("%s - %s", loc, temp);
  g_free (loc);
  g_free (temp);

  g_critical ("%s", msg);

  g_set_error (err, IPATCH_ERROR, IPATCH_ERROR_PROGRAM,
	       "Programmer error! (%s)", msg);
  g_free (msg);
}

/**
 * ipatch_strconcat_num: (skip)
 * @src: Source string
 * @num: Number to concatenate
 * @dest: Destination buffer
 * @size: Size of destination buffer
 *
 * Creates a string with a number appended to it but ensures that it is
 * of the specified @size (including NULL termination). Characters in the
 * middle of the string are removed and a ".." is inserted, if necessary.
 */
void
ipatch_strconcat_num (const char *src, int num, char *dest, int size)
{
  char numstr[16];
  int numlen, srclen, newlen, len1;
  int remove;

  sprintf (numstr, "%d", num);
  numlen = strlen (numstr);
  srclen = strlen (src);

  remove = (srclen + numlen) - (size - 1);
  if (remove > 0)	      /* any characters need to be removed? */
    {
      remove += 2;		/* for ".." */
      newlen = srclen - remove;	/* new length of non numeric string */
      len1 = (newlen + 1) / 2;	/* length of first part before ".." */

      sprintf (dest, "%.*s..%.*s%s", len1, src,
	       newlen - len1, src + (srclen - (newlen - len1)),
	       numstr);
    }
  else g_stpcpy (g_stpcpy (dest, src), numstr);
}

/**
 * ipatch_dump_object: (skip)
 * @object: Object to dump
 * @recursive: Set to %TRUE to recurse the @object children (if its a
 *   #IpatchContainer derived object).
 * @file: File to dump to or %NULL for stdout
 *
 * Dumps object info to a file for debugging purposes.
 */
void
ipatch_dump_object (GObject *object, gboolean recursive, FILE *file)
{
  char indent_buf[64] = "";

  g_return_if_fail (G_IS_OBJECT (object));
  if (!file) file = stdout;

  if (!recursive)
    {
      dump_object_info (object, indent_buf, file);
      fprintf (file, "</%s addr=%p>\n",
	       g_type_name (G_TYPE_FROM_INSTANCE (object)), object);
    }
  else dump_recursive (object, indent_buf, file);
}

static void
dump_recursive (GObject *object, char *indent, FILE *file)
{
  dump_object_info (object, indent, file);

  strcat (indent, "  ");	/* increase indent */

  if (IPATCH_IS_CONTAINER (object))
    {		 /* iterate over children if its an IpatchContainer */
      IpatchList *list;
      IpatchIter iter;
      GObject *obj;

      list = ipatch_container_get_children (IPATCH_CONTAINER (object),
					    G_TYPE_OBJECT); /* ++ ref list */
      ipatch_list_init_iter (list, &iter);

      obj = ipatch_iter_first (&iter);
      if (obj) fprintf (file, "\n");
      while (obj)
	{
	  dump_recursive (obj, indent, file);
	  obj = ipatch_iter_next (&iter);
	}
      g_object_unref (list);	/* -- unref list */
    }

  indent[strlen (indent) - 2] = '\0'; /* decrease indent */

  fprintf (file, "%s</%s>\n", indent,
	   g_type_name (G_TYPE_FROM_INSTANCE (object)));
}

static void
dump_object_info (GObject *object, char *indent, FILE *file)
{
  GParamSpec **pspecs, **pspec;
  GValue value = { 0 };
  char *contents;

  fprintf (file, "%s<%s addr=%p>\n", indent,
	   g_type_name (G_TYPE_FROM_INSTANCE (object)), object);

  fprintf (file, "%s  refcount = %u\n", indent, object->ref_count);

  pspecs = g_object_class_list_properties (G_OBJECT_GET_CLASS (object), NULL);
  pspec = pspecs;
  while (*pspec)		/* write out property values */
    {
      if ((*pspec)->flags & G_PARAM_READABLE)
	{
	  g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (*pspec));
	  g_object_get_property (object, g_param_spec_get_name (*pspec),
				 &value);
	  contents = g_strdup_value_contents (&value);
	  g_value_unset (&value);

	  fprintf (file, "%s  %s = %s\n", indent,
		   g_param_spec_get_name (*pspec), contents);
	  g_free (contents);
	}

      pspec++;
    }
  g_free (pspecs);
}

/**
 * ipatch_glist_unref_free: (skip)
 * @objlist: List of GObjects
 *
 * Unreference each GObject in a GList and free the list.
 *
 * Since: 1.1.0
 */
void
ipatch_glist_unref_free (GList *objlist)
{
  g_list_free_full (objlist, g_object_unref);
}
