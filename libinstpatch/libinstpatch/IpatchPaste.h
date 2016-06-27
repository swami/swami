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
#ifndef __IPATCH_PASTE_H__
#define __IPATCH_PASTE_H__

#include <stdarg.h>
#include <glib.h>
#include <glib-object.h>
#include <libinstpatch/IpatchContainer.h>
#include <libinstpatch/IpatchItem.h>
#include <libinstpatch/IpatchList.h>

/* forward type declarations */

typedef struct _IpatchPaste IpatchPaste;
typedef struct _IpatchPasteClass IpatchPasteClass;

#define IPATCH_TYPE_PASTE   (ipatch_paste_get_type ())
#define IPATCH_PASTE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), IPATCH_TYPE_PASTE, IpatchPaste))
#define IPATCH_PASTE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), IPATCH_TYPE_PASTE, IpatchPasteClass))
#define IPATCH_IS_PASTE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_PASTE))
#define IPATCH_IS_PASTE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), IPATCH_TYPE_PASTE))
#define IPATCH_PASTE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), IPATCH_TYPE_PASTE, IpatchPasteClass))

/* paste instance */
struct _IpatchPaste
{
  GObject parent_instance;	/* derived from GObject */

  /* only available during a paste operation */
  IpatchItem *dest;		/* current destination */

  /*< private >*/

  GSList *add_list;	        /* list of AddItemBag operations */
  GSList *add_list_last;        /* last item in add_list for append optimization */
  GHashTable *add_hash;	/* hash of original -> AddItemBag (see IpatchPaste.c) */

  GSList *link_list;	/* list of LinkItemBag operations (in reverse order) */
};

/* conversion class */
struct _IpatchPasteClass
{
  GObjectClass parent_class;
};

/* choice values for when a item conflict occurs */
typedef enum
{
  IPATCH_PASTE_CHOICE_IGNORE,	/* item will be pasted (conflict remains) */
  IPATCH_PASTE_CHOICE_REPLACE,	/* replace item */
  IPATCH_PASTE_CHOICE_KEEP,	/* keep existing item (reverse replace) */
  IPATCH_PASTE_CHOICE_CANCEL	/* cancel the current operation */
} IpatchPasteChoice;

/**
 * IpatchPasteTestFunc:
 * @dest: Destination item of paste operation
 * @src: Source item of paste operation
 *
 * Test if a paste handler can handle the paste operation.
 *
 * Returns: %TRUE if paste supported by this handler, %FALSE otherwise
 */
typedef gboolean (*IpatchPasteTestFunc)(IpatchItem *dest, IpatchItem *src);

/**
 * IpatchPasteExecFunc:
 * @paste: Paste object
 * @dest: Destination item of paste operation
 * @src: Source item of paste operation
 * @err: Location to store error information
 *
 * Perform the construction phase of the paste operation.  This includes every
 * action up to the point of actually adding/linking objects.  All object
 * addition and linking operations are stored in @paste instance to be executed
 * after resolving conflicts, etc.
 *
 * Returns: %TRUE on success, %FALSE on error (in which case @err may be set).
 */
typedef gboolean (*IpatchPasteExecFunc)(IpatchPaste *paste, IpatchItem *dest,
					IpatchItem *src, GError **err);

/**
 * IpatchPasteResolveFunc:
 * @paste: Paste instance
 * @conflict: Existing conflict item
 * @item: Conflicting item being pasted
 *
 * Function type used to handle paste item conflicts.
 *
 * Returns: Return a choice value for how the conflict should be handled.
 */
typedef IpatchPasteChoice (*IpatchPasteResolveFunc)
  (IpatchPaste *paste, IpatchItem *conflict, IpatchItem *item);

/* priority levels for paste handlers */
typedef enum
{
  /* 0 value is an alias for IPATCH_PASTE_PRIORITY_DEFAULT */

  IPATCH_PASTE_PRIORITY_LOWEST  = 1,
  IPATCH_PASTE_PRIORITY_LOW     = 25,
  IPATCH_PASTE_PRIORITY_DEFAULT = 50,
  IPATCH_PASTE_PRIORITY_HIGH    = 75,
  IPATCH_PASTE_PRIORITY_HIGHEST = 100
} IpatchPastePriority;

#define IPATCH_PASTE_FLAGS_PRIORITY_MASK   0x7F


void ipatch_register_paste_handler (IpatchPasteTestFunc test_func,
				    IpatchPasteExecFunc exec_func, int flags);
int
ipatch_register_paste_handler_full (IpatchPasteTestFunc test_func,
			            IpatchPasteExecFunc exec_func,
			            GDestroyNotify notify_func,
                                    gpointer user_data, int flags);

gboolean ipatch_is_paste_possible (IpatchItem *dest, IpatchItem *src);
gboolean ipatch_simple_paste (IpatchItem *dest, IpatchItem *src, GError **err);

GType ipatch_paste_get_type (void);
IpatchPaste *ipatch_paste_new (void);
gboolean ipatch_paste_objects (IpatchPaste *paste, IpatchItem *dest,
			       IpatchItem *src, GError **err);
gboolean ipatch_paste_resolve (IpatchPaste *paste,
			       IpatchPasteResolveFunc resolve_func,
			       gpointer user_data);
gboolean ipatch_paste_finish (IpatchPaste *paste, GError **err);
IpatchList *ipatch_paste_get_add_list (IpatchPaste *paste);

void ipatch_paste_object_add (IpatchPaste *paste, IpatchItem *additem,
			      IpatchContainer *parent, IpatchItem *orig);
IpatchItem *ipatch_paste_object_add_duplicate (IpatchPaste *paste,
					       IpatchItem *item,
					       IpatchContainer *parent);
IpatchItem *ipatch_paste_object_add_duplicate_deep (IpatchPaste *paste,
						    IpatchItem *item,
						    IpatchContainer *parent);
gboolean ipatch_paste_object_add_convert (IpatchPaste *paste,
					  GType conv_type, IpatchItem *item,
					  IpatchContainer *parent,
                                          IpatchList **item_list,
					  GError **err);
void ipatch_paste_object_link (IpatchPaste *paste, IpatchItem *from,
			       IpatchItem *to);

gboolean ipatch_paste_default_test_func (IpatchItem *dest, IpatchItem *src);
gboolean ipatch_paste_default_exec_func (IpatchPaste *paste, IpatchItem *dest,
					 IpatchItem *src, GError **err);
#endif
