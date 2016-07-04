/*
 * libInstPatch
 * Copyright (C) 1999-2014 Element Green <element@elementsofsound.org>
 *
 * Author of this file: (C) 2012 BALATON Zoltan <balaton@eik.bme.hu>
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
 * SECTION: IpatchSLIFile
 * @short_description: Spectralis file object
 * @see_also: 
 *
 * A #IpatchFile object type for Spectralis instrument and instrument collection
 * files.
 */
#include <string.h>
#include <glib.h>
#include <glib-object.h>
#include "IpatchSLIFile.h"
#include "IpatchSLIFile_priv.h"
#include "ipatch_priv.h"

static gboolean ipatch_sli_file_identify (IpatchFile *file,
                                          IpatchFileHandle *handle, GError **err);

G_DEFINE_TYPE (IpatchSLIFile, ipatch_sli_file, IPATCH_TYPE_FILE);


/* Spectralis file class init function */
static void
ipatch_sli_file_class_init (IpatchSLIFileClass *klass)
{
  IpatchFileClass *file_class = IPATCH_FILE_CLASS (klass);

  file_class->identify = ipatch_sli_file_identify;
}

static void
ipatch_sli_file_init (IpatchSLIFile *file)
{
}

/* Spectralis file identification method */
static gboolean
ipatch_sli_file_identify (IpatchFile *file, IpatchFileHandle *handle, GError **err)
{
  guint32 buf[3];
  char *filename;
  int len;

  if (handle)   /* Test content */
  {
    if (!ipatch_file_read (handle, buf, 12, err))
      return (FALSE);

    if (buf[0] == IPATCH_SLI_FOURCC_SIFI && buf[2] == 0x100)
      return (TRUE);
  }  /* Test file name extension */
  else if ((filename = ipatch_file_get_name (file)))    /* ++ alloc file name */
  {
    len = strlen (filename);

    if (len >= 4 &&
        (g_ascii_strcasecmp (filename + len - 4, ".slc") == 0 ||
         g_ascii_strcasecmp (filename + len - 4, ".sli") == 0))
    {
      g_free (filename);        /* -- free file name */
      return (TRUE);
    }

    g_free (filename);        /* -- free file name */
  }

  return (FALSE);
}

/**
 * ipatch_sli_file_new:
 *
 * Create a new Spectralis file object.
 *
 * Returns: New Spectralis file object (derived from IpatchFile) with a
 * reference count of 1. Caller owns the reference and removing it will
 * destroy the item.
 */
IpatchSLIFile *
ipatch_sli_file_new (void)
{
  return (IPATCH_SLI_FILE (g_object_new (IPATCH_TYPE_SLI_FILE, NULL)));
}
