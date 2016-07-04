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
#ifndef __IPATCH_UNIT_H__
#define __IPATCH_UNIT_H__

#include <glib.h>
#include <glib-object.h>

typedef struct _IpatchUnitInfo IpatchUnitInfo;

/* structure defining a unit type */
struct _IpatchUnitInfo
{
  guint16 id;			/* unit type ID */
  guint8 digits;	   	/* significant digits to display to user */
  guint8 flags;			/* IpatchUnitFlags */
  GType value_type;		/* unit value type */
  char *name;			/* name identifier (constant) */
  char *label;			/* unit label (translated) */
  char *descr;			/* unit description (translated) */
};

typedef enum
{
  IPATCH_UNIT_LOGARITHMIC = 1 << 0, /* unit is logarithmic */
  IPATCH_UNIT_USER = 1 << 1	/* a user friendly unit type */
} IpatchUnitFlags;

/**
 * IpatchValueTransform:
 * @src: Source value to transform from
 * @dest: Destination value to transform to
 *
 * Transform from one value to another.  The @src and @dest values have
 * already been initialized to specific types and the transform function
 * should convert/process them as necessary.
 */
typedef void (*IpatchValueTransform)(const GValue *src, GValue *dest);

/* built-in unit types */
typedef enum
{
  IPATCH_UNIT_TYPE_NONE = 0,	/* a NULL value */
  IPATCH_UNIT_TYPE_INT = 1,
  IPATCH_UNIT_TYPE_UINT = 2,
  IPATCH_UNIT_TYPE_RANGE = 3,
  IPATCH_UNIT_TYPE_DECIBELS = 4,
  IPATCH_UNIT_TYPE_PERCENT = 5,
  IPATCH_UNIT_TYPE_SEMITONES = 6,
  IPATCH_UNIT_TYPE_CENTS = 7,
  IPATCH_UNIT_TYPE_TIME_CENTS = 8,
  IPATCH_UNIT_TYPE_SAMPLE_RATE = 9,
  IPATCH_UNIT_TYPE_SAMPLES = 10,
  IPATCH_UNIT_TYPE_HERTZ = 11,
  IPATCH_UNIT_TYPE_SECONDS = 12,
  IPATCH_UNIT_TYPE_MULTIPLIER = 13,

  /* 128 - 159 reserved for IpatchUnit_DLS.h */
  IPATCH_UNIT_TYPE_DLS_GAIN = 128,
  IPATCH_UNIT_TYPE_DLS_ABS_TIME = 129,
  IPATCH_UNIT_TYPE_DLS_REL_TIME = 130,
  IPATCH_UNIT_TYPE_DLS_ABS_PITCH = 131,
  IPATCH_UNIT_TYPE_DLS_REL_PITCH = 132,
  IPATCH_UNIT_TYPE_DLS_PERCENT = 133,

  /* 160 - 169 reserved for IpatchUnit_SF2.h */
  IPATCH_UNIT_TYPE_SF2_ABS_PITCH = 160,
  IPATCH_UNIT_TYPE_SF2_OFS_PITCH = 161,
  IPATCH_UNIT_TYPE_SF2_ABS_TIME = 162,
  IPATCH_UNIT_TYPE_SF2_OFS_TIME = 163,
  IPATCH_UNIT_TYPE_CENTIBELS = 164,
  IPATCH_UNIT_TYPE_32K_SAMPLES = 165,
  IPATCH_UNIT_TYPE_TENTH_PERCENT = 166
} IpatchUnitType;

/*
 * Unit class types define domains of conversion, an example is the "user"
 * unit class which is used to convert values to units digestable by a human.
 * A conversion class is essentially a mapping between unit types, which can
 * then be used to lookup conversion functions.
 */
typedef enum
{
  IPATCH_UNIT_CLASS_NONE,	/* a NULL value */
  IPATCH_UNIT_CLASS_USER,   /* "user" conversion class (for humans) */
  IPATCH_UNIT_CLASS_DLS,	/* DLS (native patch type) class */
  IPATCH_UNIT_CLASS_COUNT   /* NOT A VALID CLASS - count of classes */
} IpatchUnitClassType;


GType ipatch_unit_info_get_type (void);
IpatchUnitInfo *ipatch_unit_info_new (void);
void ipatch_unit_info_free (IpatchUnitInfo *info);
IpatchUnitInfo *ipatch_unit_info_duplicate (const IpatchUnitInfo *info);
guint16 ipatch_unit_register (const IpatchUnitInfo *info);
IpatchUnitInfo *ipatch_unit_lookup (guint16 id);
IpatchUnitInfo *ipatch_unit_lookup_by_name (const char *name);
void ipatch_unit_class_register_map (guint16 class_type, guint16 src_units,
				     guint16 dest_units);
IpatchUnitInfo *ipatch_unit_class_lookup_map (guint16 class_type,
					      guint16 src_units);
void ipatch_unit_conversion_register (guint16 src_units, guint16 dest_units,
				      IpatchValueTransform func);
void ipatch_unit_conversion_register_full (guint16 src_units, guint16 dest_units,
                                           IpatchValueTransform func,
                                           GDestroyNotify notify_func, gpointer user_data);
IpatchValueTransform ipatch_unit_conversion_lookup (guint16 src_units,
						    guint16 dest_units,
						    gboolean *set);
gboolean ipatch_unit_convert (guint16 src_units, guint16 dest_units,
			      const GValue *src_val, GValue *dest_val);
double ipatch_unit_user_class_convert (guint16 src_units,
				       const GValue *src_val);

#endif
