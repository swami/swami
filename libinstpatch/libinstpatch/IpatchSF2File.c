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
 * SECTION: IpatchSF2File
 * @short_description: SoundFont file object
 * @see_also: 
 * @stability: Stable
 *
 * A #IpatchFile object type specifically for SoundFont files.
 */
#include <string.h>
#include <glib.h>
#include <glib-object.h>
#include "IpatchSF2File.h"
#include "IpatchSF2File_priv.h"
#include "ipatch_priv.h"

/* IpatchSF2File properties */
enum
{
  PROP_FILE_0,
  PROP_FILE_SAMPLE_POS,		/* position in file of sample chunk */
  PROP_FILE_SAMPLE_SIZE,	/* size of sample chunk */
  PROP_FILE_SAMPLE24_POS   /* position in file of LS bytes of 24 bit samples */
};

static void ipatch_sf2_file_set_property (GObject *object, guint property_id,
					  const GValue *value,
					  GParamSpec *pspec);
static void ipatch_sf2_file_get_property (GObject *object, guint property_id,
					  GValue *value, GParamSpec *pspec);
static gboolean ipatch_sf2_file_identify (IpatchFile *file,
                                          IpatchFileHandle *handle, GError **err);
static gboolean ipatch_sf2_file_real_set_sample_pos (IpatchSF2File *file,
						     guint sample_pos);
static gboolean ipatch_sf2_file_real_set_sample_size (IpatchSF2File *file,
						      guint sample_size);
static gboolean ipatch_sf2_file_real_set_sample24_pos (IpatchSF2File *file,
						       guint sample24_pos);

G_DEFINE_TYPE (IpatchSF2File, ipatch_sf2_file, IPATCH_TYPE_FILE);


/* SoundFont file class init function */
static void
ipatch_sf2_file_class_init (IpatchSF2FileClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  IpatchFileClass *file_class = IPATCH_FILE_CLASS (klass);
  IpatchItemClass *item_class = IPATCH_ITEM_CLASS (klass);

  obj_class->get_property = ipatch_sf2_file_get_property;
  file_class->identify = ipatch_sf2_file_identify;
  item_class->item_set_property = ipatch_sf2_file_set_property;

  g_object_class_install_property (obj_class, PROP_FILE_SAMPLE_POS,
		    g_param_spec_uint ("sample-pos", "Sample Chunk Position",
				       "Position in file of sample data chunk",
				       0,
				       0xFFFFFFFF,
				       0,
				       G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_FILE_SAMPLE_SIZE,
		    g_param_spec_uint ("sample-size", "Sample Chunk Size",
				       "Size of sample data chunk, in samples",
				       0,
				       0xFFFFFFFF,
				       0,
				       G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_FILE_SAMPLE24_POS,
		    g_param_spec_uint ("sample24-pos", "Sample24 Chunk Position",
				       "Position in file of 24 bit sample chunk",
				       0,
				       0xFFFFFFFF,
				       0,
				       G_PARAM_READWRITE));
}

static void
ipatch_sf2_file_init (IpatchSF2File *file)
{
}

static void
ipatch_sf2_file_set_property (GObject *object, guint property_id,
			      const GValue *value, GParamSpec *pspec)
{
  IpatchSF2File *sfont_file = IPATCH_SF2_FILE (object);

  switch (property_id)
    {
    case PROP_FILE_SAMPLE_POS:
      ipatch_sf2_file_real_set_sample_pos (sfont_file,
					   g_value_get_uint (value));
      break;
    case PROP_FILE_SAMPLE_SIZE:
      ipatch_sf2_file_real_set_sample_size (sfont_file,
					    g_value_get_uint (value));
      break;
    case PROP_FILE_SAMPLE24_POS:
      ipatch_sf2_file_real_set_sample24_pos (sfont_file,
					     g_value_get_uint (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ipatch_sf2_file_get_property (GObject *object, guint property_id,
			      GValue *value, GParamSpec *pspec)
{
  IpatchSF2File *sfont_file = IPATCH_SF2_FILE (object);

  switch (property_id)
    {
    case PROP_FILE_SAMPLE_POS:
      g_value_set_uint (value, ipatch_sf2_file_get_sample_pos (sfont_file));
      break;
    case PROP_FILE_SAMPLE_SIZE:
      g_value_set_uint (value, ipatch_sf2_file_get_sample_size (sfont_file));
      break;
    case PROP_FILE_SAMPLE24_POS:
      g_value_set_uint (value, ipatch_sf2_file_get_sample24_pos (sfont_file));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

/* SoundFont file identification method */
static gboolean
ipatch_sf2_file_identify (IpatchFile *file, IpatchFileHandle *handle, GError **err)
{
  guint32 buf[3];
  char *filename;
  int len;

  if (handle)   /* Test content */
  {
    if (!ipatch_file_read (handle, buf, 12, err))
      return (FALSE);

    if (buf[0] == IPATCH_FOURCC_RIFF && buf[2] == IPATCH_SFONT_FOURCC_SFBK)
      return (TRUE);
  }  /* Test file name extension */
  else if ((filename = ipatch_file_get_name (file)))    /* ++ alloc file name */
  {
    len = strlen (filename);

    if (len >= 4 && g_ascii_strcasecmp (filename + len - 4, ".sf2") == 0)
    {
      g_free (filename);        /* -- free file name */
      return (TRUE);
    }

    g_free (filename);        /* -- free file name */
  }

  return (FALSE);
}

/**
 * ipatch_sf2_file_new:
 *
 * Create a new SoundFont file object.
 *
 * Returns: New SoundFont file object (derived from IpatchFile) with a
 * reference count of 1. Caller owns the reference and removing it will
 * destroy the item.
 */
IpatchSF2File *
ipatch_sf2_file_new (void)
{
  return (IPATCH_SF2_FILE (g_object_new (IPATCH_TYPE_SF2_FILE, NULL)));
}

/**
 * ipatch_sf2_file_set_sample_pos:
 * @file: SoundFont file object to set position of sample chunk
 * @sample_pos: Position in the SoundFont file of the first sample of the
 *   sample data chunk, in bytes
 *
 * Sets the position of the sample data chunk in a SoundFont file object.
 */
void
ipatch_sf2_file_set_sample_pos (IpatchSF2File *file, guint sample_pos)
{
  if (ipatch_sf2_file_real_set_sample_pos (file, sample_pos))
    g_object_notify (G_OBJECT (file), "sample-pos");
}

/* the real SoundFont file set sample chunk position routine */
static gboolean
ipatch_sf2_file_real_set_sample_pos (IpatchSF2File *file, guint sample_pos)
{
  g_return_val_if_fail (IPATCH_IS_SF2_FILE (file), FALSE);

  /* atomic write */
  file->sample_pos = sample_pos;

  return (TRUE);
}

/**
 * ipatch_sf2_file_get_sample_pos:
 * @file: SoundFont file object to get position of sample chunk from
 *
 * Gets the position of the sample data chunk in a SoundFont file object.
 *
 * Returns: Position in the SoundFont file of the first sample of the
 *   sample data chunk, in bytes
 */
guint
ipatch_sf2_file_get_sample_pos (IpatchSF2File *file)
{
  guint pos;

  g_return_val_if_fail (IPATCH_IS_SF2_FILE (file), 0);

  /* atomic read */
  pos = file->sample_pos;

  return (pos);
}

/**
 * ipatch_sf2_file_set_sample_size:
 * @file: SoundFont file object to set the size of the sample chunk
 * @sample_size: Size of the sample data chunk, in samples
 *
 * Sets the size of the sample data chunk in a SoundFont file object.
 */
void
ipatch_sf2_file_set_sample_size (IpatchSF2File *file, guint sample_size)
{
  if (ipatch_sf2_file_real_set_sample_size (file, sample_size))
    g_object_notify (G_OBJECT (file), "sample-size");
}

/* the real SoundFont file set sample chunk size routine */
static gboolean
ipatch_sf2_file_real_set_sample_size (IpatchSF2File *file,
				      guint sample_size)
{
  g_return_val_if_fail (IPATCH_IS_SF2_FILE (file), FALSE);

  /* atomic write */
  file->sample_size = sample_size;

  return (TRUE);
}

/**
 * ipatch_sf2_file_get_sample_size:
 * @file: SoundFont file object to get the size of the sample chunk from
 *
 * Gets the size of the sample data chunk in a SoundFont file object.
 *
 * Returns: Size of the sample data chunk, in samples
 */
guint
ipatch_sf2_file_get_sample_size (IpatchSF2File *file)
{
  guint size;

  g_return_val_if_fail (IPATCH_IS_SF2_FILE (file), 0);

  /* atomic read */
  size = file->sample_size;

  return (size);
}

/**
 * ipatch_sf2_file_set_sample24_pos:
 * @file: SoundFont file object to set position of sample24 chunk
 * @sample24_pos: Position in the SoundFont file of the first sample of the
 *   sample 24 data chunk, in bytes
 *
 * Sets the position of the sample 24 data chunk in a SoundFont file object.
 * This optional chunk contains the lower significant bytes of 24 bit samples.
 */
void
ipatch_sf2_file_set_sample24_pos (IpatchSF2File *file, guint sample24_pos)
{
  if (ipatch_sf2_file_real_set_sample24_pos (file, sample24_pos))
    g_object_notify (G_OBJECT (file), "sample24-pos");
}

/* the real SoundFont file set sample24 chunk position routine */
static gboolean
ipatch_sf2_file_real_set_sample24_pos (IpatchSF2File *file, guint sample24_pos)
{
  g_return_val_if_fail (IPATCH_IS_SF2_FILE (file), FALSE);

  /* atomic write */
  file->sample24_pos = sample24_pos;

  return (TRUE);
}

/**
 * ipatch_sf2_file_get_sample24_pos:
 * @file: SoundFont file object to get position of sample24 chunk from
 *
 * Gets the position of the sample24 data chunk in a SoundFont file object.
 *
 * Returns: Position in the SoundFont file of the first byte of the
 *   sample24 data chunk, in bytes
 */
guint
ipatch_sf2_file_get_sample24_pos (IpatchSF2File *file)
{
  g_return_val_if_fail (IPATCH_IS_SF2_FILE (file), 0);

  /* atomic read */
  return (file->sample24_pos);
}
