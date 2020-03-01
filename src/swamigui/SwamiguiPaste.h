/*
 * SwamiguiPaste.h - Header file for Swami item paste object
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
#ifndef __SWAMIGUI_PASTE_H__
#define __SWAMIGUI_PASTE_H__

/* FIXME - Put me somewhere useful
 *
 * Paste methods should iterate over the list of items and determine
 * if it can handle the source to destination paste operation and
 * update the status of the @paste context to #SWAMIGUI_PASTE_UNHANDLED
 * if it cannot. If handled, then the paste operation should be
 * carried out while checking for conflicts. If a conflict is
 * encountered the @paste context should be set to
 * #SWAMIGUI_PASTE_CONFLICT and the function should return.
 * swamigui_paste_save_state() can be used to save state data of each
 * function in the call chain to allow the operation to resume after a
 * decision is made.
 */

typedef struct _SwamiguiPaste SwamiguiPaste;
typedef struct _SwamiguiPasteClass SwamiguiPasteClass;

#define SWAMIGUI_TYPE_PASTE   (swamigui_paste_get_type ())
#define SWAMIGUI_PASTE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWAMIGUI_TYPE_PASTE, SwamiguiPaste))
#define SWAMIGUI_PASTE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), SWAMIGUI_TYPE_PASTE, SwamiguiPasteClass))
#define SWAMIGUI_IS_PASTE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWAMIGUI_TYPE_PASTE))
#define SWAMIGUI_IS_PASTE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), SWAMIGUI_TYPE_PASTE))

/* Status of a patch item paste operation */
typedef enum
{
    SWAMIGUI_PASTE_NORMAL,		/* system normal */
    SWAMIGUI_PASTE_ERROR,		/* an error has occured */
    SWAMIGUI_PASTE_UNHANDLED,	/* unhandled paste types */
    SWAMIGUI_PASTE_CONFLICT,	/* a conflict occured, choice required */
    SWAMIGUI_PASTE_CANCEL		/* cancel paste operation */
} SwamiguiPasteStatus;

typedef enum
{
    SWAMIGUI_PASTE_NO_DECISION = 0,
    SWAMIGUI_PASTE_SKIP = 1 << 0,	/* skip item (keep old conflict item) */
    SWAMIGUI_PASTE_CHANGED = 1 << 1, /* item change (check for conflicts, etc) */
    SWAMIGUI_PASTE_REPLACE = 1 << 2 /* replace conflict item */
} SwamiguiPasteDecision;

/* Swami paste object */
struct _SwamiguiPaste
{
    GObject parent_instance;	/* derived from GObject */

    SwamiguiPasteStatus status;	/* current status of paste */
    SwamiguiPasteDecision decision; /* decision value (set for conflicts) */
    int decision_mask; /* a mask of allowable decisions for a conflict */

    IpatchItem *dstitem;		/* paste destination item */
    GList *srcitems;		/* source items (IpatchItem types) */
    GList *curitem;		/* current source item being processed */
    GHashTable *item_hash;	/* hash of item relations (choices) */

    GList *states; /* state stack for methods (so we can resume operations) */

    IpatchItem *conflict_src;	/* source conflict item */
    IpatchItem *conflict_dst;	/* destination conflict item */
};

/* Swami paste class */
struct _SwamiguiPasteClass
{
    GObjectClass parent_class;
};

GType swamigui_paste_get_type(void);
SwamiguiPaste *swamigui_paste_new(void);
gboolean swamigui_paste_process(SwamiguiPaste *paste);
void swamigui_paste_set_items(SwamiguiPaste *paste, IpatchItem *dstitem,
                              GList *srcitems);
void swamigui_paste_get_conflict_items(SwamiguiPaste *paste, IpatchItem **src,
                                       IpatchItem **dest);
void swamigui_paste_set_conflict_items(SwamiguiPaste *paste, IpatchItem *src,
                                       IpatchItem *dest);
void swamigui_paste_push_state(SwamiguiPaste *paste, gpointer state);
gpointer swamigui_paste_pop_state(SwamiguiPaste *paste);

#endif
