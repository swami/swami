/*
 * value_transform.c - Value transform functions
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
#include <stdlib.h>
#include <glib-object.h>

static void value_transform_string_int (const GValue *src_value,
					GValue *dest_value);
static void value_transform_string_double (const GValue *src_value,
					   GValue *dest_value);

void
_swami_value_transform_init (void)
{
  g_value_register_transform_func (G_TYPE_STRING, G_TYPE_INT,
				   value_transform_string_int);
  g_value_register_transform_func (G_TYPE_STRING, G_TYPE_DOUBLE,
				   value_transform_string_double);
}

/* string to int transform function */
static void
value_transform_string_int (const GValue *src_value, GValue *dest_value)
{
  const char *str;
  char *endptr;
  long int lval = 0;

  str = g_value_get_string (src_value);

  if (str)
  {
    lval = strtol (str, &endptr, 10);
    if (*endptr != '\0' || endptr == str) lval = 0;
  }

  g_value_set_int (dest_value, lval);
}

/* string to double transform function */
static void
value_transform_string_double (const GValue *src_value, GValue *dest_value)
{
  const char *str;
  char *endptr;
  double dval = 0.0;

  str = g_value_get_string (src_value);

  if (str)
  {
    dval = strtod (str, &endptr);
    if (*endptr != '\0' || endptr == str) dval = 0;
  }

  g_value_set_double (dest_value, dval);
}
