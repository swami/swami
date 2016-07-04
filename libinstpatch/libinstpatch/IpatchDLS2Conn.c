/*
 * libInstPatch
 * Copyright (C) 1999-2014 Element Green <element@elementsofsound.org>
 *
 * This program is free software; you can redistribute it and/or
 * connify it under the terms of the GNU Conneral Public License
 * as published by the Free Software Foundation; version 2.1
 * of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Conneral Public License for more details.
 *
 * You should have received a copy of the GNU Conneral Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA or on the web at http://www.gnu.org.
 */
/**
 * SECTION: IpatchDLS2Conn
 * @short_description: DLS version 2 connection structures and functions
 * @see_also: 
 * @stability: Stable
 *
 * Defines structures and functions used for DLS version 2 instrument
 * parameters (called connections in DLS terminology).
 */
#include <glib.h>
#include <glib-object.h>
#include "IpatchDLS2Conn.h"
#include "ipatch_priv.h"


/**
 * ipatch_dls2_conn_get_type:
 *
 * Get the #IpatchDLS2Conn boxed type
 *
 * Returns: Boxed GType of the #IpatchDLS2Conn structure
 */
GType
ipatch_dls2_conn_get_type (void)
{
  static GType type = 0;

  if (!type)
    type = g_boxed_type_register_static ("IpatchDLS2Conn",
				(GBoxedCopyFunc)ipatch_dls2_conn_duplicate,
				(GBoxedFreeFunc)ipatch_dls2_conn_free);

  return (type);
}

/**
 * ipatch_dls2_conn_new: (skip)
 *
 * Create a new connection
 *
 * Returns: New connection
 */
IpatchDLS2Conn *
ipatch_dls2_conn_new (void)
{
  return (g_slice_new0 (IpatchDLS2Conn));
}

/**
 * ipatch_dls2_conn_free: (skip)
 * @conn: Connection to free, should not be referenced by any zones.
 *
 * Free an #IpatchDLS2Conn structure
 */
void
ipatch_dls2_conn_free (IpatchDLS2Conn *conn)
{
  g_slice_free (IpatchDLS2Conn, conn);
}

/**
 * ipatch_dls2_conn_duplicate: (skip)
 * @conn: DLS connection to duplicate
 *
 * Duplicate a connection
 *
 * Returns: New duplicate connection
 */
IpatchDLS2Conn *
ipatch_dls2_conn_duplicate (const IpatchDLS2Conn *conn)
{
  IpatchDLS2Conn *newconn;

  g_return_val_if_fail (conn != NULL, NULL);

  newconn = ipatch_dls2_conn_new ();

  newconn->src = conn->src;
  newconn->ctrlsrc = conn->ctrlsrc;
  newconn->dest = conn->dest;
  newconn->trans = conn->trans;
  newconn->scale = conn->scale;

  return (newconn);
}

/**
 * ipatch_dls2_conn_list_set:
 * @list: (element-type IpatchDLS2Conn) (transfer none): Pointer to the root pointer of a
 *   connection list
 * @conn: DLS connection to set in @list
 *
 * Set a connection in a connection list. The connection list is searched for
 * any existing identical connection (same source, control and destination).
 * If an identical connection is found, its values are overwritten with the
 * new values, otherwise a new connection is added to the list and the values
 * copied to it.
 */
void
ipatch_dls2_conn_list_set (GSList **list, const IpatchDLS2Conn *conn)
{
  GSList *p, *last = NULL;
  IpatchDLS2Conn *c;

  g_return_if_fail (list != NULL);
  g_return_if_fail (conn != NULL);

  p = *list;
  while (p)
    {
      c = (IpatchDLS2Conn *)(p->data);
      if (IPATCH_DLS2_CONN_ARE_IDENTICAL (c, conn)) break;
      last = p;
      p = g_slist_next (p);
    }

  if (!p)			/* duplicate not found? */
    {
      c = ipatch_dls2_conn_duplicate (conn);

      if (last) last = g_slist_append (last, c);  /* assign to supress gcc warning */
      else *list = g_slist_append (NULL, c);
    }
  else *c = *conn;		/* overwrite old connection values */
}

/**
 * ipatch_dls2_conn_list_unset:
 * @list: (element-type IpatchDLS2Conn) (transfer none): Pointer to the root pointer of a
 *   connection list
 * @conn: DLS connection to remove from @list
 *
 * Remove a connection from a connection list. The connection list is
 * searched for an identical connection to @conn (same source,
 * control and destination). If a match is found, it is removed, otherwise
 * nothing. This essentially sets a connection to its default value, for
 * those connections which are defined.
 */
void
ipatch_dls2_conn_list_unset (GSList **list, const IpatchDLS2Conn *conn)
{
  IpatchDLS2Conn *c;
  GSList *p, *prev = NULL;

  g_return_if_fail (list != NULL);
  g_return_if_fail (conn != NULL);

  p = *list;
  while (p)
    {
      c = (IpatchDLS2Conn *)(p->data);
      if (IPATCH_DLS2_CONN_ARE_IDENTICAL (c, conn))
	{			/* free and remove connection */
	  ipatch_dls2_conn_free (c);
	  if (!prev) *list = p->next;
	  else prev->next = p->next;
	  g_slist_free_1 (p);
	  return;
	}
      prev = p;
      p = g_slist_next (p);
    }
}

/**
 * ipatch_dls2_conn_list_duplicate:
 * @list: (element-type IpatchDLS2Conn) (transfer none): GSList of #IpatchDLS2Conn structures
 *   to duplicate
 *
 * Duplicates a connection list (GSList and connection data).
 *
 * Returns: (element-type IpatchDLS2Conn) (transfer full): New duplicate connection list which
 * should be freed with ipatch_dls2_conn_list_free() when finished with it.
 */
GSList *
ipatch_dls2_conn_list_duplicate (const GSList *list)
{
  GSList *newlist = NULL;

  while (list)
    {
      newlist = g_slist_prepend (newlist, ipatch_dls2_conn_duplicate
				((IpatchDLS2Conn *)(list->data)));
      list = list->next;
    }

  return (g_slist_reverse (newlist));
}

/**
 * ipatch_dls2_conn_list_duplicate_fast: (skip)
 * @list: (element-type IpatchDLS2Conn): GSList of #IpatchDLS2Conn structures
 *   to duplicate
 *
 * Like ipatch_dls2_conn_list_duplicate() but optimized for speed, new list
 * is backwards from original.
 *
 * Returns: (element-type IpatchDLS2Conn) (transfer full): New duplicate connection list which
 *   should be freed with ipatch_dls2_conn_list_free() when finished with it.
 */
GSList *
ipatch_dls2_conn_list_duplicate_fast (const GSList *list)
{
  GSList *newlist = NULL;

  while (list)
    {
      newlist = g_slist_prepend (newlist, ipatch_dls2_conn_duplicate
				((IpatchDLS2Conn *)(list->data)));
      list = list->next;
    }

  return (newlist);
}

/**
 * ipatch_dls2_conn_list_free: (skip)
 * @list: (element-type IpatchDLS2Conn): GSList of #IpatchDLS2Conn structures
 *   to free
 * @free_conns: If %TRUE then the connections themselves are freed, %FALSE
 *   makes this function act just like g_slist_free() (only the list is
 *   freed not the connections).
 *
 * Free a list of connections
 */
void
ipatch_dls2_conn_list_free (GSList *list, gboolean free_conns)
{
  GSList *p;

  if (free_conns)
    {
      p = list;
      while (p)
	{
	  ipatch_dls2_conn_free ((IpatchDLS2Conn *)(p->data));
	  p = g_slist_delete_link (p, p);
	}
    }
  else g_slist_free (list);
}
