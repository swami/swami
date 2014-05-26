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
#ifndef __IPATCH_DLS2_INFO_H__
#define __IPATCH_DLS2_INFO_H__

#include <glib.h>
#include <glib-object.h>
#include <libinstpatch/IpatchRiff.h>
#include <libinstpatch/IpatchDLSFile.h>

typedef GSList IpatchDLS2Info;
typedef struct _IpatchDLS2InfoBag IpatchDLS2InfoBag;

/* a container for a INFO ID and value (generally not accessed directly) */
struct _IpatchDLS2InfoBag
{
  guint32 fourcc;		/* FOURCC int ID */
  char *value;			/* info string value */
};

/* known DLS2 INFO FOURCC IDs */
typedef enum
{
  IPATCH_DLS2_ARCHIVE_LOCATION = IPATCH_DLS_FOURCC_IARL,
  IPATCH_DLS2_ARTIST           = IPATCH_DLS_FOURCC_IART,
  IPATCH_DLS2_COMMISSIONED     = IPATCH_DLS_FOURCC_ICMS,
  IPATCH_DLS2_COMMENT          = IPATCH_DLS_FOURCC_ICMT,
  IPATCH_DLS2_COPYRIGHT        = IPATCH_DLS_FOURCC_ICOP,
  IPATCH_DLS2_DATE             = IPATCH_DLS_FOURCC_ICRD,
  IPATCH_DLS2_ENGINEER         = IPATCH_DLS_FOURCC_IENG,
  IPATCH_DLS2_GENRE            = IPATCH_DLS_FOURCC_IGNR,
  IPATCH_DLS2_KEYWORDS         = IPATCH_DLS_FOURCC_IKEY,
  IPATCH_DLS2_MEDIUM           = IPATCH_DLS_FOURCC_IMED,
  IPATCH_DLS2_NAME             = IPATCH_DLS_FOURCC_INAM,
  IPATCH_DLS2_PRODUCT          = IPATCH_DLS_FOURCC_IPRD,
  IPATCH_DLS2_SUBJECT          = IPATCH_DLS_FOURCC_ISBJ,
  IPATCH_DLS2_SOFTWARE         = IPATCH_DLS_FOURCC_ISFT,
  IPATCH_DLS2_SOURCE           = IPATCH_DLS_FOURCC_ISRC,
  IPATCH_DLS2_SOURCE_FORM      = IPATCH_DLS_FOURCC_ISRF,
  IPATCH_DLS2_TECHNICIAN       = IPATCH_DLS_FOURCC_ITCH
} IpatchDLS2InfoType;


char *ipatch_dls2_info_get (IpatchDLS2Info *info, guint32 fourcc);
G_CONST_RETURN char *ipatch_dls2_info_peek (IpatchDLS2Info *info,
					    guint32 fourcc);
void ipatch_dls2_info_set (IpatchDLS2Info **info, guint32 fourcc,
			   const char *value);
void ipatch_dls2_info_free (IpatchDLS2Info *info);
IpatchDLS2Info *ipatch_dls2_info_duplicate (IpatchDLS2Info *info);

gboolean ipatch_dls2_info_is_defined (guint32 fourcc);

void ipatch_dls2_info_install_class_properties (GObjectClass *obj_class);
gboolean ipatch_dls2_info_set_property (IpatchDLS2Info **info_list,
					guint property_id,
					const GValue *value);
gboolean ipatch_dls2_info_get_property (IpatchDLS2Info *info_list,
					guint property_id,
					GValue *value);
void ipatch_dls2_info_notify (IpatchItem *item, guint32 fourcc,
			      const GValue *new_value, const GValue *old_value);

IpatchDLS2InfoBag *ipatch_dls2_info_bag_new (void);
void ipatch_dls2_info_bag_free (IpatchDLS2InfoBag *bag);

#endif
