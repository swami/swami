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
 * SECTION: IpatchSampleStoreSndFile
 * @short_description: Sample store object type which uses libsndfile to access
 *   audio in sound files
 * @see_also: 
 * @stability: Stable
 */
#include <string.h>
#include <glib.h>
#include <glib-object.h>

#include "IpatchSampleStoreSndFile.h"
#include "IpatchSampleStore.h"
#include "IpatchSndFile.h"
#include "ipatch_priv.h"
#include "builtin_enums.h"
#include "i18n.h"

/*
 * Notes:
 *
 * In read mode the audio file must be identified before being opened for
 * reading.  This is necessary in order to assign the correct #IpatchSample:format
 * to the sample store, based on the file's format.
 *
 * PCM formats are read and written using sf_read/write_raw to allow for more
 * flexibility and to remove the need to do extra conversions (in the case of
 * 8 bit, 24 bit or non CPU endian formats).
 *
 * Non-PCM formats are read/written as 16 bit CPU endian order data.
 */

enum
{
  PROP_0,
  PROP_TITLE,
  PROP_LOOP_TYPE,
  PROP_LOOP_START,
  PROP_LOOP_END,
  PROP_ROOT_NOTE,
  PROP_FINE_TUNE,

  PROP_FILE_NAME,
  PROP_FILE_FORMAT,
  PROP_SUB_FORMAT,
  PROP_ENDIAN
};

static void ipatch_sample_store_snd_file_sample_iface_init (IpatchSampleIface *iface);
static void ipatch_sample_store_snd_file_get_title (IpatchSampleStoreSndFile *store,
                                                    GValue *value);
static void ipatch_sample_store_snd_file_set_property
  (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static void ipatch_sample_store_snd_file_get_property
  (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void ipatch_sample_store_snd_file_finalize (GObject *object);
static gboolean ipatch_sample_store_snd_file_sample_iface_open
  (IpatchSampleHandle *handle, GError **err);
static gboolean verify_read_format (IpatchSampleStoreSndFile *store);
static gboolean verify_write_format (IpatchSampleStoreSndFile *store);
static void ipatch_sample_store_snd_file_sample_iface_close (IpatchSampleHandle *handle);
static gboolean ipatch_sample_store_snd_file_sample_iface_read
  (IpatchSampleHandle *handle, guint offset, guint frames, gpointer buf, GError **err);
static gboolean ipatch_sample_store_snd_file_sample_iface_write
  (IpatchSampleHandle *handle, guint offset, guint frames, gconstpointer buf, GError **err);
static int libsndfile_read_format_convert (int sndfile_format, int channels, gboolean *raw);
static int libsndfile_write_format_convert (int sndfile_format, int channels);


/* Supported loop types */
static int ipatch_sample_store_snd_file_loop_types[] = {
  IPATCH_SAMPLE_LOOP_NONE,
  IPATCH_SAMPLE_LOOP_STANDARD,
  IPATCH_SAMPLE_LOOP_PINGPONG,
  IPATCH_SAMPLE_LOOP_TYPE_TERM	/* terminator */
};



G_DEFINE_TYPE_WITH_CODE (IpatchSampleStoreSndFile, ipatch_sample_store_snd_file,
                         IPATCH_TYPE_SAMPLE_STORE,
                         G_IMPLEMENT_INTERFACE (IPATCH_TYPE_SAMPLE,
                                                ipatch_sample_store_snd_file_sample_iface_init))

static void
ipatch_sample_store_snd_file_sample_iface_init (IpatchSampleIface *iface)
{
  iface->open = ipatch_sample_store_snd_file_sample_iface_open;
  iface->close = ipatch_sample_store_snd_file_sample_iface_close;
  iface->read = ipatch_sample_store_snd_file_sample_iface_read;
  iface->write = ipatch_sample_store_snd_file_sample_iface_write;
  iface->loop_types = ipatch_sample_store_snd_file_loop_types;
}

static void
ipatch_sample_store_snd_file_class_init (IpatchSampleStoreSndFileClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  IpatchItemClass *item_class = IPATCH_ITEM_CLASS (klass);
  GType file_format, file_sub_format;

  obj_class->finalize = ipatch_sample_store_snd_file_finalize;
  obj_class->get_property = ipatch_sample_store_snd_file_get_property;
  item_class->item_set_property = ipatch_sample_store_snd_file_set_property;

  g_object_class_override_property (obj_class, PROP_TITLE, "title");

  ipatch_sample_install_property (obj_class, PROP_LOOP_TYPE, "loop-type");
  ipatch_sample_install_property (obj_class, PROP_LOOP_START, "loop-start");
  ipatch_sample_install_property (obj_class, PROP_LOOP_END, "loop-end");
  ipatch_sample_install_property (obj_class, PROP_ROOT_NOTE, "root-note");
  ipatch_sample_install_property (obj_class, PROP_FINE_TUNE, "fine-tune");

  g_object_class_install_property (obj_class, PROP_FILE_NAME,
                g_param_spec_string ("file-name", "File name",
                                     "File name", NULL, G_PARAM_READWRITE));

  file_format = ipatch_snd_file_format_get_type ();

  g_object_class_install_property (obj_class, PROP_FILE_FORMAT,
       g_param_spec_enum ("file-format", "File format", "File format",
                          file_format, IPATCH_SND_FILE_DEFAULT_FORMAT,
                          G_PARAM_READWRITE));

  file_sub_format = ipatch_snd_file_sub_format_get_type ();

  g_object_class_install_property (obj_class, PROP_SUB_FORMAT,
       g_param_spec_enum ("sub-format", "File sub format", "File sub audio format",
                          file_sub_format, IPATCH_SND_FILE_DEFAULT_SUB_FORMAT,
                          G_PARAM_READWRITE));

  g_object_class_install_property (obj_class, PROP_ENDIAN,
       g_param_spec_enum ("endian", "Endian byte order", "Endian byte order of file",
                          IPATCH_TYPE_SND_FILE_ENDIAN,
                          IPATCH_SND_FILE_DEFAULT_ENDIAN, G_PARAM_READWRITE));
}

static void
ipatch_sample_store_snd_file_get_title (IpatchSampleStoreSndFile *store,
                                        GValue *value)
{
  char *filename, *s, *basename = NULL;

  g_object_get (store, "file-name", &filename, NULL);    /* ++ alloc filename */

  if (filename)
  {
    basename = g_path_get_basename (filename);  /* ++ alloc basename */

    s = strrchr (basename, '.'); /* search for dot delimiter */
    if (s && s > basename) *s = '\0';	/* terminate string at dot */

    g_free (filename);          /* -- free filename */
  }

  g_value_take_string (value, basename);        /* !! caller takes over alloc */
}

static void
ipatch_sample_store_snd_file_set_property (GObject *object, guint property_id,
                                           const GValue *value, GParamSpec *pspec)
{
  IpatchSampleStoreSndFile *store = IPATCH_SAMPLE_STORE_SND_FILE (object);

  /* No locking needed - all values should be set prior to multi-thread use */

  switch (property_id)
  {
    case PROP_LOOP_TYPE:
      store->loop_type = g_value_get_enum (value);
      break;
    case PROP_LOOP_START:
      store->loop_start = g_value_get_uint (value);
      break;
    case PROP_LOOP_END:
      store->loop_end = g_value_get_uint (value);
      break;
    case PROP_ROOT_NOTE:
      store->root_note = g_value_get_int (value);
      break;
    case PROP_FINE_TUNE:
      store->fine_tune = g_value_get_int (value);
      break;
    case PROP_FILE_NAME:
      g_free (store->filename);
      store->filename = g_value_dup_string (value);

      /* IpatchItem notify for "title" property */
      {
        GValue titleval = { 0 };
        g_value_init (&titleval, G_TYPE_STRING);
        ipatch_sample_store_snd_file_get_title (store, &titleval);
        ipatch_item_prop_notify ((IpatchItem *)store, ipatch_item_pspec_title,
                                 &titleval, NULL);
        g_value_unset (&titleval);
      }
      break;
    case PROP_FILE_FORMAT:
      store->file_format = g_value_get_enum (value);
      break;
    case PROP_SUB_FORMAT:
      store->sub_format = g_value_get_enum (value);
      break;
    case PROP_ENDIAN:
      store->endian = g_value_get_enum (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
ipatch_sample_store_snd_file_get_property (GObject *object, guint property_id,
                                           GValue *value, GParamSpec *pspec)
{
  IpatchSampleStoreSndFile *store = IPATCH_SAMPLE_STORE_SND_FILE (object);

  /* No locking needed - all values should be set prior to multi-thread use */

  switch (property_id)
  {
    case PROP_TITLE:
      ipatch_sample_store_snd_file_get_title (store, value);
      break;
    case PROP_LOOP_TYPE:
      g_value_set_enum (value, store->loop_type);
      break;
    case PROP_LOOP_START:
      g_value_set_uint (value, store->loop_start);
      break;
    case PROP_LOOP_END:
      g_value_set_uint (value, store->loop_end);
      break;
    case PROP_ROOT_NOTE:
      g_value_set_int (value, store->root_note);
      break;
    case PROP_FINE_TUNE:
      g_value_set_int (value, store->fine_tune);
      break;
    case PROP_FILE_NAME:
      g_value_set_string (value, store->filename);
      break;
    case PROP_FILE_FORMAT:
      g_value_set_enum (value, store->file_format);
      break;
    case PROP_SUB_FORMAT:
      g_value_set_enum (value, store->sub_format);
      break;
    case PROP_ENDIAN:
      g_value_set_enum (value, store->endian);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
ipatch_sample_store_snd_file_init (IpatchSampleStoreSndFile *sample)
{
  sample->file_format = IPATCH_SND_FILE_DEFAULT_FORMAT;
  sample->sub_format = IPATCH_SND_FILE_DEFAULT_SUB_FORMAT;
  sample->endian = IPATCH_SND_FILE_DEFAULT_ENDIAN;
  sample->loop_type = IPATCH_SAMPLE_LOOP_NONE;
  sample->root_note = IPATCH_SAMPLE_ROOT_NOTE_DEFAULT;
}

static void
ipatch_sample_store_snd_file_finalize (GObject *object)
{
  IpatchSampleStoreSndFile *sample = IPATCH_SAMPLE_STORE_SND_FILE (object);

  g_free (sample->filename);

  if (G_OBJECT_CLASS (ipatch_sample_store_snd_file_parent_class)->finalize)
    G_OBJECT_CLASS (ipatch_sample_store_snd_file_parent_class)->finalize (object);
}

static gboolean
ipatch_sample_store_snd_file_sample_iface_open (IpatchSampleHandle *handle,
                                                GError **err)
{
  IpatchSampleStoreSndFile *store = IPATCH_SAMPLE_STORE_SND_FILE (handle->sample);
  SF_INFO info;
  int format;

  g_return_val_if_fail (store->filename != NULL, FALSE);

  format = ipatch_sample_store_get_format (store);

  if (handle->read_mode)
  {
    if (!store->identified)
    {
      g_set_error (err, IPATCH_ERROR, IPATCH_ERROR_INVALID,
                   _("Sample file '%s' has not yet been identified for reading"),
                   store->filename);
      return (FALSE);
    }

    if (!verify_read_format (store))
    {
      g_set_error (err, IPATCH_ERROR, IPATCH_ERROR_INVALID,
                   _("Invalid read format values for sample file '%s'"),
                   store->filename);
      return (FALSE);
    }

    if (store->raw)
    {
      handle->data2 = sf_read_raw;    /* Just read raw data (conversions handled internally) */
      handle->data3 = GUINT_TO_POINTER (ipatch_sample_format_size (format));
    }
    else
    {
      switch (IPATCH_SAMPLE_FORMAT_GET_WIDTH (format))
      {
        case IPATCH_SAMPLE_16BIT:
          handle->data2 = sf_readf_short;
          break;
        case IPATCH_SAMPLE_32BIT:
          handle->data2 = sf_readf_int;
          break;
        case IPATCH_SAMPLE_FLOAT:
          handle->data2 = sf_readf_float;
          break;
        case IPATCH_SAMPLE_DOUBLE:
          handle->data2 = sf_readf_double;
          break;
        default:
          g_set_error (err, IPATCH_ERROR, IPATCH_ERROR_INVALID,
                       _("Inconsistent state for sample file '%s'"),
                       store->filename);
          return (FALSE);
      }

      handle->data3 = GUINT_TO_POINTER (1);  /* Multiplier is 1, for number of frames */
    }
  }
  else  /* Write mode */
  {
    if (!verify_write_format (store))
    {
      g_set_error (err, IPATCH_ERROR, IPATCH_ERROR_INVALID,
                   _("Invalid write format values for sample file '%s'"),
                   store->filename);
      return (FALSE);
    }

    switch (IPATCH_SAMPLE_FORMAT_GET_WIDTH (format))
    {
      case IPATCH_SAMPLE_8BIT:
        handle->data2 = sf_write_raw;    /* Just write raw data (conversions handled internally) */
        handle->data3 = GUINT_TO_POINTER (ipatch_sample_format_size (format));
        break;
      case IPATCH_SAMPLE_16BIT:
        handle->data2 = sf_writef_short;
        handle->data3 = GUINT_TO_POINTER (1);
        break;
      case IPATCH_SAMPLE_32BIT:
        handle->data2 = sf_writef_int;
        handle->data3 = GUINT_TO_POINTER (1);
        break;
      case IPATCH_SAMPLE_FLOAT:
        handle->data2 = sf_writef_float;
        handle->data3 = GUINT_TO_POINTER (1);
        break;
      case IPATCH_SAMPLE_DOUBLE:
        handle->data2 = sf_writef_double;
        handle->data3 = GUINT_TO_POINTER (1);
        break;
    }
  }

  /* Write mode? - Fill in format structure */
  if (!handle->read_mode)
  {
    info.samplerate = ((IpatchSampleStore *)store)->rate;
    info.channels = IPATCH_SAMPLE_FORMAT_GET_CHANNEL_COUNT (format);

    info.format = store->file_format | store->sub_format | store->endian;

    if (!sf_format_check (&info))
    {
      g_set_error (err, IPATCH_ERROR, IPATCH_ERROR_INVALID,
                   _("Invalid libsndfile format for file '%s' (format: 0x%08X, chan: %d, rate: %d)"),
                   store->filename, info.format, info.channels, info.samplerate);
      return (FALSE);
    }
  }
  else info.format = 0;

  /* Open the file using libsndfile */
  handle->data1 = sf_open (store->filename,
                           handle->read_mode ? SFM_READ : SFM_WRITE, &info);
  if (!handle->data1)
  {
    if (handle->read_mode)
      g_set_error (err, IPATCH_ERROR, IPATCH_ERROR_IO,
                   _("Error opening file '%s' for reading"), store->filename);
    else
      g_set_error (err, IPATCH_ERROR, IPATCH_ERROR_IO,
                   _("Error opening file '%s' for writing"), store->filename);
    return (FALSE);
  }

  /* Store instrument info if write mode and not default values */
  if (!handle->read_mode
      && (store->loop_type != IPATCH_SAMPLE_LOOP_NONE
          || store->root_note != IPATCH_SAMPLE_ROOT_NOTE_DEFAULT
          || store->fine_tune != 0))
  {
    SF_INSTRUMENT instinfo;

    memset (&instinfo, 0, sizeof (instinfo));

    instinfo.basenote = store->root_note;
    instinfo.detune = store->fine_tune;
    instinfo.velocity_lo = 0;
    instinfo.velocity_hi = 127;
    instinfo.key_lo = 0;
    instinfo.key_hi = 127;

    if (store->loop_type != IPATCH_SAMPLE_LOOP_NONE)
    {
      instinfo.loop_count = 1;

      switch (store->loop_type)
      {
        case IPATCH_SAMPLE_LOOP_PINGPONG:
          instinfo.loops[0].mode = SF_LOOP_ALTERNATING;
          break;
        case IPATCH_SAMPLE_LOOP_STANDARD:
        case IPATCH_SAMPLE_LOOP_RELEASE:
        default:
          instinfo.loops[0].mode = SF_LOOP_FORWARD;
          break;
      }

      instinfo.loops[0].start = store->loop_start;
      instinfo.loops[0].end = store->loop_end;
    }

    sf_command (handle->data1, SFC_SET_INSTRUMENT, &instinfo, sizeof (instinfo));
  }

  /* Used as current offset (in frames) into file to optimize out seek calls and prevent
   * failure when attempting unnecessary seeks in formats which don't support it. */
  handle->data4 = GUINT_TO_POINTER (0);

  return (TRUE);
}

/* Verify that libsndfile and IpatchSample format parameters are consistent for reading */
static gboolean
verify_read_format (IpatchSampleStoreSndFile *store)
{
  int format, conv_format;
  gboolean raw;

  format = ipatch_sample_store_get_format (store);
  conv_format = libsndfile_read_format_convert (store->file_format | store->sub_format | store->endian,
                                                IPATCH_SAMPLE_FORMAT_GET_CHANNEL_COUNT (format), &raw);

  return (format == conv_format && raw == store->raw);
}

/* Verify that libsndfile and IpatchSample format parameters are consistent for writing */
static gboolean
verify_write_format (IpatchSampleStoreSndFile *store)
{
  int format, conv_format;

  format = ipatch_sample_store_get_format (store);
  conv_format = libsndfile_write_format_convert (store->file_format | store->sub_format | store->endian,
                                                 IPATCH_SAMPLE_FORMAT_GET_CHANNEL_COUNT (format));

  return (format == conv_format);
}

static void
ipatch_sample_store_snd_file_sample_iface_close (IpatchSampleHandle *handle)
{
  if (handle->data1)
  {
    sf_close (handle->data1);
    handle->data1 = NULL;
  }
}

static gboolean
ipatch_sample_store_snd_file_sample_iface_read (IpatchSampleHandle *handle,
                                                guint offset, guint frames,
                                                gpointer buf, GError **err)
{
  IpatchSampleStoreSndFile *sample = (IpatchSampleStoreSndFile *)(handle->sample);
  SNDFILE *sfhandle = (SNDFILE *)(handle->data1);
  sf_count_t (*readfunc)(SNDFILE *sndfile, gpointer buf, sf_count_t count) = handle->data2;
  guint count = frames * GPOINTER_TO_UINT (handle->data3);      /* data3 is frames multiplier */
  guint filepos = GPOINTER_TO_UINT (handle->data4);     /* data4 is current offset */
  sf_count_t read_count;

  if (offset != filepos && sf_seek (sfhandle, offset, SEEK_SET) == -1)
  {
    g_set_error (err, IPATCH_ERROR, IPATCH_ERROR_IO,
                 _("libsndfile error seeking file '%s': %s"), sample->filename,
                 sf_strerror (sfhandle));
    return (FALSE);
  }

  read_count = readfunc (sfhandle, buf, count);

  if (read_count != count)
  {
    if (sf_error (sfhandle) == SF_ERR_NO_ERROR)
      g_set_error (err, IPATCH_ERROR, IPATCH_ERROR_UNEXPECTED_EOF,
                   _("Unexpected end of file in '%s'"), sample->filename);
    else
      g_set_error (err, IPATCH_ERROR, IPATCH_ERROR_IO,
                   _("libsndfile error reading file '%s': %s"), sample->filename,
                   sf_strerror (sfhandle));
    return (FALSE);
  }

  filepos += frames;
  handle->data4 = GUINT_TO_POINTER (filepos);

  return (TRUE);
}

static gboolean
ipatch_sample_store_snd_file_sample_iface_write (IpatchSampleHandle *handle,
                                                 guint offset, guint frames,
                                                 gconstpointer buf, GError **err)
{
  IpatchSampleStoreSndFile *sample = (IpatchSampleStoreSndFile *)(handle->sample);
  SNDFILE *sfhandle = (SNDFILE *)(handle->data1);
  sf_count_t (*writefunc)(SNDFILE *sndfile, gconstpointer buf, sf_count_t count) = handle->data2;
  guint count = frames * GPOINTER_TO_UINT (handle->data3);      /* data3 is frames multiplier */

  if (sf_seek (sfhandle, offset, SEEK_SET) == -1)
  {
    g_set_error (err, IPATCH_ERROR, IPATCH_ERROR_IO,
                 _("libsndfile error seeking file '%s': %s"), sample->filename,
                 sf_strerror (sfhandle));
    return (FALSE);
  }

  if (writefunc (sfhandle, buf, count) != count)
  {
    g_set_error (err, IPATCH_ERROR, IPATCH_ERROR_IO,
                 _("libsndfile error writing file '%s': %s"), sample->filename,
                 sf_strerror (sfhandle));
    return (FALSE);
  }

  return (TRUE);
}

/**
 * ipatch_sample_store_snd_file_new:
 * @filename: File name to assign to the new libsndfile sample store
 *
 * Creates a new libsndfile sample store.  ipatch_sample_store_snd_file_init_read()
 * or ipatch_sample_store_snd_file_init_write() must be called, depending on
 * audio file mode, prior to opening the sample.
 *
 * Returns: (type IpatchSampleStoreSndFile): New libsndfile sample store, cast
 *   as a #IpatchSample for convenience.
 */
IpatchSample *
ipatch_sample_store_snd_file_new (const char *filename)
{
  return (IPATCH_SAMPLE (g_object_new (IPATCH_TYPE_SAMPLE_STORE_SND_FILE,
                                       "file-name", filename, NULL)));
}

/**
 * ipatch_sample_store_snd_file_init_read:
 * @store: libsndfile sample store
 *
 * Initialize a libsndfile sample store for reading.  Should be called prior to
 * opening the sample store and after the filename has been assigned.  Fills in
 * the #IpatchSampleStoreSndFile:file-format, #IpatchSampleStoreSndFile:sub-format,
 * #IpatchSampleStoreSndFile:endian, #IpatchSample:sample-rate and
 * #IpatchSample:sample-size information properties.  In addition the
 * #IpatchSample:sample-format property is set to a value for optimal loading of the
 * audio data (least amount of conversion necessary to yield uncompressed PCM
 * audio), which will be the audio format of the sample store.
 *
 * Returns: %TRUE on success, %FALSE otherwise
 */
gboolean
ipatch_sample_store_snd_file_init_read (IpatchSampleStoreSndFile *store)
{
  SF_INSTRUMENT instinfo;
  SNDFILE *sfhandle;
  SF_INFO info;
  int format;
  

  memset (&info, 0, sizeof (SF_INFO));

  /* Open the file for reading using libsndfile */
  sfhandle = sf_open (store->filename, SFM_READ, &info);

  if (!sfhandle) return (FALSE);

  format = libsndfile_read_format_convert (info.format, info.channels, &store->raw);

  g_object_set (store,
                "file-format", (int)(info.format & SF_FORMAT_TYPEMASK),
                "sub-format", (int)(info.format & SF_FORMAT_SUBMASK),
                "endian", (int)(info.format & SF_FORMAT_ENDMASK),
                "sample-rate", (int)(info.samplerate),
                "sample-size", (guint)(info.frames),
                "sample-format", (int)(format),
                NULL);

  if (sf_command (sfhandle, SFC_GET_INSTRUMENT, &instinfo, sizeof (instinfo)))
  {
    int loop_type;
    unsigned int loop_start, loop_end;

    if (instinfo.loop_count > 0)
    {
      switch (instinfo.loops[0].mode)
      {
        case SF_LOOP_NONE:
          loop_type = IPATCH_SAMPLE_LOOP_NONE;
          break;
        case SF_LOOP_ALTERNATING:
          loop_type = IPATCH_SAMPLE_LOOP_PINGPONG;
          break;
        case SF_LOOP_FORWARD:
        case SF_LOOP_BACKWARD:          /* FIXME - Is there really a loop backward mode? */
        default:
          loop_type = IPATCH_SAMPLE_LOOP_STANDARD;
          break;
      }

      loop_start = instinfo.loops[0].start;
      loop_end = instinfo.loops[0].end;
    }
    else
    {
      loop_type = IPATCH_SAMPLE_LOOP_NONE;
      loop_start = loop_end = 0;
    }

    g_object_set (store,
                  "root-note", (int)(instinfo.basenote),
                  "fine-tune", (int)(instinfo.detune),
                  "loop-type", (int)(loop_type),
                  "loop-start", (guint)(loop_start),
                  "loop-end", (guint)(loop_end),
                  NULL);
  }

  sf_close (sfhandle);

  store->identified = TRUE;

  return (TRUE);
}

/**
 * ipatch_sample_store_snd_file_init_write:
 * @store: libsndfile sample store
 * @file_format: libsndfile file format type (GEnum "IpatchSndFileFormat")
 * @sub_format: libsndfile audio format type (GEnum "IpatchSndFileSubFormat")
 * @endian: libsndfile endian selection (#IpatchSampleStoreSndFileEndian)
 * @channels: Number of channels (1-8, 1=mono, 2=stereo, etc)
 * @samplerate: Audio sample rate
 *
 * Initialize a libsndfile sample store for writing.  Should be called prior to
 * opening the sample store.  The #IpatchSampleStoreSndFile:file-format,
 * #IpatchSampleStoreSndFile:sub-format, #IpatchSampleStoreSndFile:endian
 * and #IpatchSample:sample-rate properties will be assigned the provided values.
 * In addition the #IpatchSample:sample-format property is set to a value for optimal
 * writing of the audio data (including the @channels value), which will be the
 * audio format of the sample store.
 *
 * Returns: %TRUE if format variables are valid, %FALSE otherwise
 */
gboolean
ipatch_sample_store_snd_file_init_write (IpatchSampleStoreSndFile *store,
                                         int file_format, int sub_format, int endian,
                                         int channels, int samplerate)
{
  SF_INFO info;
  int sample_format;

  g_return_val_if_fail (IPATCH_IS_SAMPLE_STORE_SND_FILE (store), FALSE);
  g_return_val_if_fail (channels >= 1 && channels <= 8, FALSE);

  info.samplerate = samplerate;
  info.channels = channels;
  info.format = file_format | sub_format | endian;

  if (!sf_format_check (&info)) return (FALSE);

  sample_format = libsndfile_write_format_convert (info.format, channels);

  g_object_set (store,
                "file-format", file_format,
                "sub-format", sub_format,
                "endian", endian,
                "sample-rate", samplerate,
                "sample-format", sample_format,
                NULL);
  return (TRUE);
}

/**
 * libsndfile_read_format_convert:
 * @sndfile_format: libsndfile file/audio format
 * @channels: Number of channels (1-8, 1=mono, 2=stereo, etc)
 * @raw: Location to store boolean value indicating if data should be read raw,
 *   or %NULL to ignore
 *
 * "convert" a libsndfile format to a compatible libinstpatch format for reading.
 *
 * Returns: A compatible libinstpatch sample format
 */
static int
libsndfile_read_format_convert (int sndfile_format, int channels, gboolean *raw)
{
  gboolean rawval;
  int file_format;
  int format;

  file_format = sndfile_format & SF_FORMAT_TYPEMASK;

  /* Some formats return PCM sub formats even if they aren't actually RAW PCM,
   * such as FLAC.  This is bummer dude, since we must upconvert 24 bit to 32 bit,
   * 8 bit to 16 bit, etc or revert to nasty conversions requiring a second
   * buffer.  We trust that certain formats will have RAW PCM that we can just
   * read ourselves and do whatever we like with it.  An unfortunate limitation of
   * libsndfile. */
  if (file_format == SF_FORMAT_WAV || file_format == SF_FORMAT_AIFF)
    rawval = TRUE;
  else rawval = FALSE;

  switch (sndfile_format & SF_FORMAT_SUBMASK)
  {
    case SF_FORMAT_PCM_S8:
      format = rawval ? IPATCH_SAMPLE_8BIT
        : IPATCH_SAMPLE_16BIT | IPATCH_SAMPLE_ENDIAN_HOST;
      break;
    case SF_FORMAT_PCM_16:
      format = rawval ? IPATCH_SAMPLE_16BIT
        : IPATCH_SAMPLE_16BIT | IPATCH_SAMPLE_ENDIAN_HOST;
      break;
    case SF_FORMAT_PCM_24:
      format = rawval ? IPATCH_SAMPLE_REAL24BIT
        : IPATCH_SAMPLE_32BIT | IPATCH_SAMPLE_ENDIAN_HOST;
      break;
    case SF_FORMAT_PCM_32:
      format = rawval ? IPATCH_SAMPLE_32BIT
        : IPATCH_SAMPLE_32BIT | IPATCH_SAMPLE_ENDIAN_HOST;
      break;
    case SF_FORMAT_PCM_U8:
      format = rawval ? IPATCH_SAMPLE_8BIT | IPATCH_SAMPLE_UNSIGNED
        : IPATCH_SAMPLE_16BIT | IPATCH_SAMPLE_ENDIAN_HOST;
      break;
    case SF_FORMAT_FLOAT:
      format = IPATCH_SAMPLE_FLOAT | IPATCH_SAMPLE_ENDIAN_HOST;
      rawval = FALSE;
      break;
    case SF_FORMAT_DOUBLE:
      format = IPATCH_SAMPLE_DOUBLE | IPATCH_SAMPLE_ENDIAN_HOST;
      rawval = FALSE;
      break;
    default:    /* Just read non PCM formats as 16 bit host endian */
      format = IPATCH_SAMPLE_16BIT | IPATCH_SAMPLE_ENDIAN_HOST;
      rawval = FALSE;
      break;
  }

  if (rawval)
  {
    switch (sndfile_format & SF_FORMAT_ENDMASK)
    {
      case SF_ENDIAN_LITTLE:
        format |= IPATCH_SAMPLE_LENDIAN;
        break;
      case SF_ENDIAN_BIG:
        format |= IPATCH_SAMPLE_BENDIAN;
        break;
      default:
        format |= IPATCH_SAMPLE_ENDIAN_HOST;
        break;
    }
  }

  format |= ((channels - 1) & 0x07) << IPATCH_SAMPLE_CHANNEL_SHIFT;

  if (raw) *raw = rawval;

  return (format);
}

/**
 * libsndfile_write_format_convert:
 * @sndfile_format: libsndfile file/audio format
 * @channels: Number of channels (1-8, 1=mono, 2=stereo, etc)
 *
 * "convert" a libsndfile format to a compatible libinstpatch format for writing.
 *
 * Returns: A compatible libinstpatch sample format
 */
static int
libsndfile_write_format_convert (int sndfile_format, int channels)
{
  int format;

  switch (sndfile_format & SF_FORMAT_SUBMASK)
  {
    case SF_FORMAT_PCM_S8:
      format = IPATCH_SAMPLE_8BIT;
      break;
    case SF_FORMAT_PCM_16:
      format = IPATCH_SAMPLE_16BIT | IPATCH_SAMPLE_ENDIAN_HOST;
      break;
    case SF_FORMAT_PCM_24:     /* No libsndfile function for writing 24 bit */
    case SF_FORMAT_PCM_32:
      format = IPATCH_SAMPLE_32BIT | IPATCH_SAMPLE_ENDIAN_HOST;
      break;
    case SF_FORMAT_PCM_U8:
      format = IPATCH_SAMPLE_8BIT | IPATCH_SAMPLE_UNSIGNED;
      break;
    case SF_FORMAT_FLOAT:
      format = IPATCH_SAMPLE_FLOAT | IPATCH_SAMPLE_ENDIAN_HOST;
      break;
    case SF_FORMAT_DOUBLE:
      format = IPATCH_SAMPLE_DOUBLE | IPATCH_SAMPLE_ENDIAN_HOST;
      break;
    default:    /* Just write non PCM formats as 16 bit host endian */
      format = IPATCH_SAMPLE_16BIT | IPATCH_SAMPLE_ENDIAN_HOST;
      break;
  }

  format |= ((channels - 1) & 0x07) << IPATCH_SAMPLE_CHANNEL_SHIFT;

  return (format);
}
