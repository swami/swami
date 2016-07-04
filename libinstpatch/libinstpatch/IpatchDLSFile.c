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
 * SECTION: IpatchDLSFile
 * @short_description: DLS file object and functions
 * @see_also: 
 * @stability: Stable
 *
 * Object type for DLS files and other constants and functions dealing with
 * them.
 */
#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <glib-object.h>

#include "IpatchDLSFile.h"
#include "IpatchDLSFile_priv.h"
#include "ipatch_priv.h"
#include "i18n.h"
#include "misc.h"

static gboolean ipatch_dls_file_identify (IpatchFile *file, IpatchFileHandle *handle,
                                          GError **err);

G_DEFINE_TYPE (IpatchDLSFile, ipatch_dls_file, IPATCH_TYPE_FILE);


static void
ipatch_dls_file_class_init (IpatchDLSFileClass *klass)
{
  IpatchFileClass *file_class = IPATCH_FILE_CLASS (klass);
  file_class->identify = ipatch_dls_file_identify;
}

static void
ipatch_dls_file_init (IpatchDLSFile *file)
{
}

/* DLS/gig file identification function */
static gboolean
ipatch_dls_file_identify (IpatchFile *file, IpatchFileHandle *handle, GError **err)
{
  guint32 buf[3];
  char *filename;
  gboolean retval = FALSE;
  int len = 0;  /* Silence gcc */

  filename = ipatch_file_get_name (file);       /* ++ alloc file name */

  /* Check if file ends with .gig extension first, since we can't easily
   * check by content.  Defer to gig identify method. */
  if (filename)
  {
    len = strlen (filename);
    if (len >= 4 && g_ascii_strcasecmp (filename + len - 4, ".gig") == 0)
    {
      g_free (filename);        /* -- free file name */
      return (FALSE);
    }
  }

  if (handle)   /* Test content */
  {
    if (ipatch_file_read (handle, buf, 12, err)
        && buf[0] == IPATCH_FOURCC_RIFF && buf[2] == IPATCH_DLS_FOURCC_DLS)
      retval = TRUE;
  }
  else if (filename) /* Test file name extension */
  {
    if ((len >= 4 && g_ascii_strcasecmp (filename + len - 4, ".dls") == 0)
        || (len >= 5 && g_ascii_strcasecmp (filename + len - 5, ".dls2") == 0))
      retval = TRUE;
  }

  g_free (filename);        /* -- free filename */
  return (retval);
}

/**
 * ipatch_dls_file_new:
 *
 * Create a new DLS file object.
 *
 * Returns: New DLS file object (derived from IpatchFile) with a
 * reference count of 1. Caller owns the reference and removing it will
 * destroy the item.
 */
IpatchDLSFile *
ipatch_dls_file_new (void)
{
  return (IPATCH_DLS_FILE (g_object_new (IPATCH_TYPE_DLS_FILE, NULL)));
}
