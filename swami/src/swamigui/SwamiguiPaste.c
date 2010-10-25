/*
 * SwamiguiPaste.c - Swami item paste object
 *
 * Swami
 * Copyright (C) 1999-2010 Joshua "Element" Green <jgreen@users.sourceforge.net>
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
#include <stdio.h>
#include <string.h>

#include <libinstpatch/libinstpatch.h>
#include <libswami/libswami.h>

#include "SwamiguiPaste.h"
#include "SwamiguiRoot.h"	/* for state history */
#include "i18n.h"

static void swamigui_paste_class_init (SwamiguiPasteClass *klass);
static void swamigui_paste_init (SwamiguiPaste *paste);
static void swamigui_paste_finalize (GObject *obj);

static GObjectClass *parent_class = NULL;


GType
swamigui_paste_get_type (void)
{
  static GType obj_type = 0;

  if (!obj_type)
    {
      static const GTypeInfo obj_info =
	{
	  sizeof (SwamiguiPasteClass), NULL, NULL,
	  (GClassInitFunc) swamigui_paste_class_init, NULL, NULL,
	  sizeof (SwamiguiPaste), 0,
	  (GInstanceInitFunc) swamigui_paste_init
	};

      obj_type = g_type_register_static (G_TYPE_OBJECT, "SwamiguiPaste",
					&obj_info, 0);
    }

  return (obj_type);
}

static void
swamigui_paste_class_init (SwamiguiPasteClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  obj_class->finalize = swamigui_paste_finalize;
}

static void
swamigui_paste_init (SwamiguiPaste *paste)
{
  paste->status = SWAMIGUI_PASTE_NORMAL;
  paste->item_hash =
    g_hash_table_new_full (NULL, NULL,
			   g_object_unref, /* key destroy func */
			   g_object_unref);	/* value destroy func */

  /* don't want paste objects to be saved for session */
  swami_object_clear_flags (G_OBJECT (paste), SWAMI_OBJECT_SAVE);
}

static void
swamigui_paste_finalize (GObject *obj)
{
  SwamiguiPaste *paste = SWAMIGUI_PASTE (obj);

  if (paste->curitem)		/* paste not finished? Then cancel it. */
    {
      paste->status = SWAMIGUI_PASTE_CANCEL;
      swamigui_paste_process (paste);
    }

  if (paste->dstitem) g_object_unref (paste->dstitem); /* -- unref dest item */

  if (paste->srcitems)		/* -- unref source items */
    {
      g_list_foreach (paste->srcitems, (GFunc)g_object_unref, NULL);
      g_list_free (paste->srcitems);
    }

  g_hash_table_destroy (paste->item_hash);

  /* clear conflict items to unref them */
  swamigui_paste_set_conflict_items (paste, NULL, NULL);
}

/**
 * swamigui_paste_new:
 *
 * Create a new paste object.
 *
 * Returns: New paste object with a ref count of 1.
 */
SwamiguiPaste *
swamigui_paste_new (void)
{
  return (SWAMIGUI_PASTE (g_object_new (SWAMIGUI_TYPE_PASTE, NULL)));
}

/**
 * swamigui_paste_process:
 * @paste: Swami paste object
 *
 * Run the paste process. This function uses the IpatchPaste system to handle
 * paste operations. May need to be called multiple times to complete a paste
 * operation if decisions are required (conflict paste items, unhandled types,
 * or a cancel occurs). All paste status and parameters are stored in
 * the @paste object. This function also groups all state history actions
 * of the paste operation so it can be retracted if the paste is canceled.
 *
 * Returns: %TRUE if paste finished successfully, %FALSE if a decision is
 * required or an error occured.
 */
gboolean
swamigui_paste_process (SwamiguiPaste *paste)
{
//  IpatchItemIface *iface;
  gboolean retval = TRUE;

  g_return_val_if_fail (SWAMIGUI_IS_PASTE (paste), FALSE);
  g_return_val_if_fail (paste->dstitem != NULL, FALSE);
  g_return_val_if_fail (paste->srcitems != NULL, FALSE);
  g_return_val_if_fail (paste->status != SWAMIGUI_PASTE_ERROR, FALSE);

  /* skip unhandled types if requested */
  if (paste->status == SWAMIGUI_PASTE_UNHANDLED
      && paste->decision == SWAMIGUI_PASTE_SKIP && paste->curitem)
    {
      paste->curitem = g_list_next (paste->curitem);
      paste->decision = SWAMIGUI_PASTE_NO_DECISION;
    }

  paste->status = SWAMIGUI_PASTE_NORMAL;	/* set status to normal */
  paste->decision_mask = SWAMIGUI_PASTE_SKIP | SWAMIGUI_PASTE_CHANGED
    | SWAMIGUI_PASTE_REPLACE;	/* set decision mask to all */

//  retval = ((*iface->paste)(paste->dstitem, paste));

  paste->decision = SWAMIGUI_PASTE_NO_DECISION; /* clear previous decision */

  return (retval);
}

/**
 * swamigui_paste_set_items:
 * @paste: Swami paste object
 * @dstitem: Destination item of paste operation
 * @srcitems: A list of source #IpatchItem objects
 *
 * A function to set the destination item and source item(s) of a
 * paste operation. This function can only be called once on a paste
 * object.
 */
void
swamigui_paste_set_items (SwamiguiPaste *paste, IpatchItem *dstitem,
			  GList *srcitems)
{
//  IpatchItemIface *dstitem_iface;

  g_return_if_fail (SWAMIGUI_IS_PASTE (paste));
  g_return_if_fail (IPATCH_IS_ITEM (dstitem));
  g_return_if_fail (srcitems != NULL);

  g_return_if_fail (paste->dstitem == NULL);
  g_return_if_fail (paste->srcitems == NULL);

  /* make sure we have a paste method for the destination item */
//  dstitem_iface = IPATCH_ITEM_GET_IFACE (dstitem);
//  g_return_if_fail (dstitem_iface->paste != NULL);

  g_object_ref (dstitem);	/* ++ ref destination item */
  paste->dstitem = dstitem;

  /* copy and reference source item list */
  paste->srcitems = g_list_copy (srcitems);
  g_list_foreach (paste->srcitems, (GFunc)g_object_ref, NULL); /* ++ ref */
  paste->curitem = paste->srcitems; /* set current item to first source item */
}

/**
 * swamigui_paste_get_conflict_items:
 * @paste: Swami paste object
 * @src: Location to store a pointer to the source conflict
 *   item (item's refcount is incremented) or %NULL
 * @dest: Location to store a pointer to the conflict destination
 *   item (item's refcount is incremented) or %NULL
 *
 * Get conflict items from a @paste object (paste status should be
 * #SWAMIGUI_PASTE_CONFLICT or these will be %NULL). The returned
 * items' reference counts are incremented and the caller is
 * responsible for unrefing them with g_object_unref when finished using
 * them.
 */
void
swamigui_paste_get_conflict_items (SwamiguiPaste *paste, IpatchItem **src,
				  IpatchItem **dest)
{
  g_return_if_fail (SWAMIGUI_IS_PASTE (paste));

  if (src)
    {				/* ++ ref returned src item */
      if (paste->conflict_src) g_object_ref (paste->conflict_src);
      *src = paste->conflict_src;
    }
  if (dest)
    {				/* ++ ref returned dest item */
      if (paste->conflict_dst) g_object_ref (paste->conflict_dst);
      *dest = paste->conflict_dst;
    }
}

/**
 * swamigui_paste_set_conflict_items:
 * @paste: Swami paste object
 * @src: Source conflict item or %NULL
 * @dest: Destination conflict item or %NULL
 *
 * Set conflict items in a paste object. This function will cause the paste
 * object's status to be set to #SWAMIGUI_PASTE_CONFLICT if @src and
 * @dest are not %NULL, otherwise it will be set to
 * #SWAMIGUI_PASTE_NORMAL status and the conflict items will be cleared.
 */
void
swamigui_paste_set_conflict_items (SwamiguiPaste *paste, IpatchItem *src,
				  IpatchItem *dest)
{
  g_return_if_fail (SWAMIGUI_IS_PASTE (paste));
  g_return_if_fail (!src || IPATCH_IS_ITEM (src));
  g_return_if_fail (!dest || IPATCH_IS_ITEM (dest));

  /* unref old items if set */
  if (paste->conflict_src) g_object_unref (paste->conflict_src);
  if (paste->conflict_dst) g_object_unref (paste->conflict_dst);

  if (src && dest)
    {
      paste->status = SWAMIGUI_PASTE_CONFLICT;
      g_object_ref (src);	/* ++ ref new src conflict item */
      g_object_ref (dest);	/* ++ ref new dest conflict item */
    }
  else paste->status = SWAMIGUI_PASTE_NORMAL;

  paste->conflict_src = src;
  paste->conflict_dst = dest;
}

/**
 * swamigui_paste_push_state:
 * @paste: Swami paste object
 * @state: #IpatchItem "paste" method defined state data
 *
 * This function is used by #IpatchItem interface "paste" methods to
 * store variable state data to be able to resume a paste operation (after
 * a decision is made on a conflict, etc). The @paste object stores a stack
 * of state data so a chain of functions can be resumed at a later time.
 * The methods are responsible for creating and destroying this state data
 * and the function chain will be resumed even on a cancel operation so this
 * can be accomplished properly.
 */
void
swamigui_paste_push_state (SwamiguiPaste *paste, gpointer state)
{
  g_return_if_fail (SWAMIGUI_IS_PASTE (paste));
  g_return_if_fail (state != NULL);

  paste->states = g_list_prepend (paste->states, state);
}

/**
 * swamigui_paste_pop_state:
 * @paste: Swami paste object
 *
 * This function is used by #IpatchItem interface "paste" methods to
 * retrieve the next variable state data, stored by
 * swamigui_paste_push_state(), to be able to resume a paste
 * operation. The chain of functions storing state data should call
 * this function in the reverse order in which the states were
 * pushed. The state pointer is removed from the stack after this
 * call.
 *
 * Returns: Pointer to state data which was on the top of the stack.
 */
gpointer
swamigui_paste_pop_state (SwamiguiPaste *paste)
{
  gpointer state;

  g_return_val_if_fail (SWAMIGUI_IS_PASTE (paste), NULL);
  g_return_val_if_fail (paste->states != NULL, NULL);

  state = paste->states->data;
  paste->states = g_list_delete_link (paste->states, paste->states);

  return (state);
}
