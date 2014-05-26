/*
 * SwamiRoot.h - Root Swami application object
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
#ifndef __SWAMI_ROOT_H__
#define __SWAMI_ROOT_H__

#include <glib.h>
#include <glib-object.h>
#include <libinstpatch/libinstpatch.h>

typedef struct _SwamiRoot SwamiRoot;
typedef struct _SwamiRootClass SwamiRootClass;

#include <libswami/SwamiPropTree.h>
#include <libswami/SwamiContainer.h>
#include <libswami/SwamiLock.h>

#define SWAMI_TYPE_ROOT   (swami_root_get_type ())
#define SWAMI_ROOT(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWAMI_TYPE_ROOT, SwamiRoot))
#define SWAMI_ROOT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), SWAMI_TYPE_ROOT, SwamiRootClass))
#define SWAMI_IS_ROOT(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWAMI_TYPE_ROOT))
#define SWAMI_IS_ROOT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), SWAMI_TYPE_ROOT))

struct _SwamiRoot
{
  SwamiLock parent_instance;

  SwamiContainer *patch_root;	/* instrument patch tree */

  /*< private >*/

  SwamiPropTree *proptree;	/* object property tree */

  char *patch_search_path;
  char *patch_path;		/* default path to patch files */
  char *sample_path;		/* default path to sample files */
  char *sample_format;		/* default sample format string */
  int sample_max_size; /* max sample size in MB (until big samples handled) */
  int swap_ram_size;            /* maximum size of RAM swap in MB */
};

struct _SwamiRootClass
{
  SwamiLockClass parent_class;

  /* object add signal */
  void (*object_add)(GObject *object);
};

GType swami_root_get_type (void);
SwamiRoot *swami_root_new (void);

#define swami_root_get_patch_items(swami) \
   ipatch_container_get_children (IPATCH_CONTAINER (swami->patch_root), \
				  IPATCH_TYPE_ITEM)

SwamiRoot *swami_get_root (gpointer object);

IpatchList *swami_root_get_objects (SwamiRoot *root);
void swami_root_add_object (SwamiRoot *root, GObject *object);
GObject *swami_root_new_object (SwamiRoot *root, const char *type_name);
void swami_root_prepend_object (SwamiRoot *root, GObject *parent,
				GObject *object);
#define swami_root_append_object(root, parent, object) \
  swami_root_insert_object_before (root, parent, NULL, object)
void swami_root_insert_object_before (SwamiRoot *root, GObject *parent,
				      GObject *sibling, GObject *object);

gboolean swami_root_patch_load (SwamiRoot *root, const char *filename,
				IpatchItem **item, GError **err);
gboolean swami_root_patch_save (IpatchItem *item, const char *filename,
				GError **err);

#endif
