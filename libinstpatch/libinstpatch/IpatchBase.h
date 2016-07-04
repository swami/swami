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
 *
 */
#ifndef __IPATCH_BASE_H__
#define __IPATCH_BASE_H__

#include <stdarg.h>
#include <glib.h>
#include <glib-object.h>

/* forward type declarations */

typedef struct _IpatchBase IpatchBase;
typedef struct _IpatchBaseClass IpatchBaseClass;

#include <libinstpatch/IpatchFile.h>
#include <libinstpatch/IpatchContainer.h>
#include <libinstpatch/IpatchSF2Preset.h>
#include <libinstpatch/IpatchSF2Inst.h>
#include <libinstpatch/IpatchSF2Sample.h>

#define IPATCH_TYPE_BASE   (ipatch_base_get_type ())
#define IPATCH_BASE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), IPATCH_TYPE_BASE, IpatchBase))
#define IPATCH_BASE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), IPATCH_TYPE_BASE, IpatchBaseClass))
#define IPATCH_IS_BASE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_BASE))
#define IPATCH_IS_BASE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), IPATCH_TYPE_BASE))
#define IPATCH_BASE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), IPATCH_TYPE_BASE, IpatchBaseClass))

/**
 * IpatchBaseFlags:
 * @IPATCH_BASE_CHANGED: Does the base object have any unsaved changes?
 * @IPATCH_BASE_SAVED: Has the base object ever been saved?
 */
typedef enum
{
  IPATCH_BASE_CHANGED = 1 << IPATCH_ITEM_UNUSED_FLAG_SHIFT,
  IPATCH_BASE_SAVED   = 1 << (IPATCH_ITEM_UNUSED_FLAG_SHIFT + 1)
} IpatchBaseFlags;

/* we reserve a couple flags for backwards compatible expansion */
/**
 * IPATCH_BASE_UNUSED_FLAG_SHIFT: (skip)
 */
#define IPATCH_BASE_UNUSED_FLAG_SHIFT (IPATCH_ITEM_UNUSED_FLAG_SHIFT + 4)

/* patch base object */
struct _IpatchBase
{
  IpatchContainer parent_instance; /* derived from IpatchContainer */

  /*< private >*/

  IpatchFile *file;		/* file object associated with this patch */
};

/* SoundFont class */
struct _IpatchBaseClass
{
  IpatchContainerClass parent_class;

  /* methods */
  void (*find_unused_locale)(IpatchBase *base, int *bank, int *program,
			     const IpatchItem *exclude, gboolean percussion);
  IpatchItem * (*find_item_by_locale)(IpatchBase *base, int bank, int program);
};

/**
 * IPATCH_BASE_DEFAULT_NAME: (skip)
 */
#define IPATCH_BASE_DEFAULT_NAME "Untitled"

GType ipatch_base_get_type (void);
char *ipatch_base_type_get_mime_type (GType base_type);
void ipatch_base_set_file (IpatchBase *base, IpatchFile *file);
IpatchFile *ipatch_base_get_file (IpatchBase *base);
void ipatch_base_set_file_name (IpatchBase *base, const char *file_name);
char *ipatch_base_get_file_name (IpatchBase *base);

void ipatch_base_find_unused_midi_locale (IpatchBase *base,
					  int *bank, int *program,
					  const IpatchItem *exclude,
					  gboolean percussion);
IpatchItem *ipatch_base_find_item_by_midi_locale (IpatchBase *base, int bank,
						  int program);

gboolean ipatch_base_save (IpatchBase *base, GError **err);
gboolean ipatch_base_save_to_filename (IpatchBase *base, const char *filename, GError **err);
gboolean ipatch_base_save_a_copy (IpatchBase *base, const char *filename, GError **err);
gboolean ipatch_base_close (IpatchBase *base, GError **err);
gboolean ipatch_close_base_list (IpatchList *list, GError **err);

#endif

