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
#ifndef __IPATCH_VBANK_REGION_H__
#define __IPATCH_VBANK_REGION_H__

#include <glib.h>
#include <glib-object.h>
#include <libinstpatch/IpatchItem.h>
#include <libinstpatch/IpatchRange.h>

/* forward type declarations */

typedef struct _IpatchVBankRegion IpatchVBankRegion;
typedef struct _IpatchVBankRegionClass IpatchVBankRegionClass;

#define IPATCH_TYPE_VBANK_REGION   (ipatch_vbank_region_get_type ())
#define IPATCH_VBANK_REGION(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), IPATCH_TYPE_VBANK_REGION, \
  IpatchVBankRegion))
#define IPATCH_VBANK_REGION_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), IPATCH_TYPE_VBANK_REGION, \
  IpatchVBankRegionClass))
#define IPATCH_IS_VBANK_REGION(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_VBANK_REGION))
#define IPATCH_IS_VBANK_REGION_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), IPATCH_TYPE_VBANK_REGION))
#define IPATCH_VBANK_REGION_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), IPATCH_TYPE_VBANK_REGION, \
  IpatchVBankRegionClass))

/* Virtual bank region */
struct _IpatchVBankRegion
{
  IpatchItem parent_instance;

  /*< private >*/
  IpatchItem *item; 	/* Referenced item or NULL (if unresolved) */
  char **id_props;	/* prop/val pairs which ID item or NULL (if resolved) */
  guint file_index;	/* Index of file in IpatchVBank parent (if unresolved) */

  IpatchRange note_range;	/* MIDI note range of this region */
  guint8 note_range_mode;	/* #IpatchVBankRegionNoteRangeMode */
  gint8 root_note;		/* MIDI root note value */
  guint8 root_note_mode;	/* #IpatchVBankRegionRootNoteMode */
};

struct _IpatchVBankRegionClass
{
  IpatchItemClass parent_class;
};

/**
 * IpatchVBankNoteRangeMode:
 * @IPATCH_VBANK_REGION_NOTE_RANGE_MODE_INTERSECT: Note range is applied as an
 *   intersection with existing voice note ranges (only those notes shared by
 *   both ranges will become the final range, a logic AND operation).
 * @IPATCH_VBANK_REGION_NOTE_RANGE_MODE_ASSIGN: Note range of all voices is
 *   overridden by new range.
 *
 * Determines mode in which a virtual bank region's note range is applied to the
 * affected synthesis voices.
 */
typedef enum
{
  IPATCH_VBANK_REGION_NOTE_RANGE_MODE_INTERSECT	= 0,
  IPATCH_VBANK_REGION_NOTE_RANGE_MODE_OVERRIDE	= 1
} IpatchVBankRegionNoteRangeMode;

/**
 * IpatchVBankRootNoteMode:
 * @IPATCH_VBANK_REGION_ROOT_NOTE_MODE_OFFSET: Offset the root note parameters of
 *   affected synthesis voices by a given signed integer amount.
 * @IPATCH_VBANK_REGION_ROOT_NOTE_MODE_OVERRIDE: Override root note parameters of
 *   affected synthesis voices.
 */
typedef enum
{
  IPATCH_VBANK_REGION_ROOT_NOTE_MODE_OFFSET	= 0,
  IPATCH_VBANK_REGION_ROOT_NOTE_MODE_OVERRIDE	= 1
} IpatchVBankRegionRootNoteMode;

GType ipatch_vbank_region_get_type (void);
IpatchVBankRegion *ipatch_vbank_region_new (void);

IpatchVBankRegion *ipatch_vbank_region_first (IpatchIter *iter);
IpatchVBankRegion *ipatch_vbank_region_next (IpatchIter *iter);

void ipatch_vbank_region_set_id_props (IpatchVBankRegion *region,
				       char **id_props);
char **ipatch_vbank_region_get_id_props (IpatchVBankRegion *region,
					 guint *n_elements);
void ipatch_vbank_region_set_item (IpatchVBankRegion *region, IpatchItem *item);
IpatchItem *ipatch_vbank_region_get_item (IpatchVBankRegion *region);

#endif

