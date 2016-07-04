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
 * SECTION: IpatchGigFile
 * @short_description: GigaSampler file object
 * @see_also: 
 * @stability: Stable
 *
 * File type for GigaSampler files.
 */
#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <glib-object.h>

#include "IpatchGigFile.h"
#include "IpatchGigFile_priv.h"
#include "IpatchDLSFile_priv.h"
#include "i18n.h"
#include "misc.h"

static gboolean ipatch_gig_file_identify_method (IpatchFile *file,
                                                 IpatchFileHandle *handle, GError **err);

G_DEFINE_TYPE (IpatchGigFile, ipatch_gig_file, IPATCH_TYPE_DLS_FILE);


/* GigaSampler file class init function */
static void
ipatch_gig_file_class_init (IpatchGigFileClass *klass)
{
  IpatchFileClass *file_class = IPATCH_FILE_CLASS (klass);
  file_class->identify = ipatch_gig_file_identify_method;
  /* load_object method handled by parent class IpatchDLSFile */
}

static void
ipatch_gig_file_init (IpatchGigFile *file)
{
}

/* GigaSampler file identification method
 * ^&$#@*()!!!! They went and polluted the DLS file magic namespace. Rather
 * than check the file data, we use the primitive file extension. One does
 * not actually know if its a GigaSampler file until running into one of their
 * proprietary chunks (usually 3lnk in an instrument region).
 */
static gboolean
ipatch_gig_file_identify_method (IpatchFile *file, IpatchFileHandle *handle,
                                 GError **err)
{
  char *filename;
  guint32 buf[3];
  gboolean retval = TRUE;
  int len;

  filename = ipatch_file_get_name (file);       /* ++ alloc file name */
  if (!filename) return (FALSE);

  len = strlen (filename);

  if (len < 4 || g_ascii_strcasecmp (filename + len - 4, ".gig") != 0)
    retval = FALSE;

  g_free (filename);    /* -- free file name */

  if (handle && retval)
  { /* Check for DLS signature */
    if (!ipatch_file_read (handle, buf, 12, err)
        || buf[0] != IPATCH_FOURCC_RIFF || buf[2] != IPATCH_DLS_FOURCC_DLS)
      retval = FALSE;
  }

  return (retval);
}

/**
 * ipatch_gig_file_new:
 *
 * Create a new GigaSampler file object.
 *
 * Returns: New GigaSampler file object with a reference count of 1.
 * Caller owns the reference and removing it will destroy the item.
 */
IpatchGigFile *
ipatch_gig_file_new (void)
{
  return (IPATCH_GIG_FILE (g_object_new (IPATCH_TYPE_GIG_FILE, NULL)));
}
