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
#ifndef __IPATCH_RANGE_H__
#define __IPATCH_RANGE_H__

#include <glib.h>
#include <glib-object.h>

typedef struct _IpatchRange IpatchRange;
typedef struct _IpatchParamSpecRange IpatchParamSpecRange;

#define IPATCH_TYPE_RANGE   (ipatch_range_get_type ())
#define IPATCH_VALUE_HOLDS_RANGE(value) \
  (G_TYPE_CHECK_VALUE_TYPE ((value), IPATCH_TYPE_RANGE))

/* integer range structure */
struct _IpatchRange
{
  int low;		/* low endpoint of range or -1 if undefined */
  int high;	       /* high endpoint of range or -1 if undefined */
};

/* set range value making sure that are in the correct order */
#define IPATCH_RANGE_SET_VALUES(range, val1, val2) G_STMT_START { \
  if (val1 <= val2) \
    { \
      range.low = val1; \
      range.high = val2; \
    } \
  else \
    { \
      range.low = val2; \
      range.high = val1; \
    } \
} G_STMT_END

/* set a range value to a NULL range (an undefined value) */
#define IPATCH_RANGE_SET_NULL(range) G_STMT_START { \
  range.low = range.high = -1; \
} G_STMT_END


GType ipatch_range_get_type (void);
IpatchRange *ipatch_range_new (int low, int high);
IpatchRange *ipatch_range_copy (IpatchRange *range);
void ipatch_range_free (IpatchRange *range);

void ipatch_value_set_range (GValue *value, const IpatchRange *range);
void ipatch_value_set_static_range (GValue *value, IpatchRange *range);
IpatchRange *ipatch_value_get_range (const GValue *value);

/* range parameter specification */

#define IPATCH_TYPE_PARAM_RANGE (ipatch_param_spec_range_get_type ())
#define IPATCH_IS_PARAM_SPEC_RANGE(pspec) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), IPATCH_TYPE_PARAM_RANGE))
#define IPATCH_PARAM_SPEC_RANGE(pspec) \
  (G_TYPE_CHECK_INSTANCE_CAST ((pspec), IPATCH_TYPE_PARAM_RANGE, \
   IpatchParamSpecRange))

/* a parameter specification for the integer range type */
struct _IpatchParamSpecRange
{
  GParamSpec parent_instance;	/* derived from GParamSpec */
  int min, max;		  /* min and max values for range endpoints */
  int default_low, default_high; /* default vals for low and high endpoints */
};

GType ipatch_param_spec_range_get_type (void);
GParamSpec *ipatch_param_spec_range (const char *name, const char *nick,
				     const char *blurb,
				     int min, int max,
				     int default_low, int default_high,
				     GParamFlags flags);

#endif
