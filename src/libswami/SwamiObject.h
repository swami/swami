/*
 * SwamiObject.h - Child object properties and type rank systems
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
#ifndef __SWAMI_OBJECT_H__
#define __SWAMI_OBJECT_H__

#include <glib.h>
#include <glib-object.h>
#include <libinstpatch/IpatchList.h>

#include <libswami/SwamiRoot.h>

typedef struct _SwamiObjectPropBag SwamiObjectPropBag;

/* structure used for Swami object properties */
struct _SwamiObjectPropBag
{
    SwamiRoot *root;		/* parent swami root object */
    char *name;			/* Swami object property name */
    guint8 rank;			/* Swami object rank property */
    guint8 flags;			/* Swami object flags property */
    guint16 reserved;
};

/* some pre-defined ranks (valid range is 1-100) */
typedef enum
{
    SWAMI_RANK_INVALID = 0,
    SWAMI_RANK_LOWEST  = 10,
    SWAMI_RANK_LOW     = 25,
    SWAMI_RANK_NORMAL  = 50,	/* NORMAL default value */
    SWAMI_RANK_DEFAULT = 60,	/* value to elect default objects */
    SWAMI_RANK_HIGH    = 75,
    SWAMI_RANK_HIGHEST = 90
} SwamiRank;

typedef enum /*< flags >*/
{
    SWAMI_OBJECT_SAVE = 1 << 0,	/* flags if object state should be saved */
    SWAMI_OBJECT_USER = 1 << 1	/* user visable object (in tree view, etc) */
} SwamiObjectFlags;

extern GQuark swami_object_propbag_quark;

void swami_type_set_rank(GType type, GType group_type, int rank);
int swami_type_get_rank(GType type, GType group_type);
GType *swami_type_get_children(GType group_type);
GType swami_type_get_default(GType group_type);

void swami_object_set_default(GObject *object, GType type);
GObject *swami_object_get_by_name(GObject *object, const char *name);
IpatchList *swami_object_find_by_type(GObject *object, const char *type_name);
GObject *swami_object_get_by_type(GObject *object, const char *type_name);

void swami_object_get_valist(GObject *object, const char *first_property_name,
                             va_list var_args);
void swami_object_get_property(GObject *object, const char *property_name,
                               GValue *value);
void swami_object_get(gpointer object, const char *first_prop_name, ...);

void swami_object_set_valist(GObject *object, const char *first_property_name,
                             va_list var_args);
void swami_object_set_property(GObject *object, const char *property_name,
                               const GValue *value);
void swami_object_set(gpointer object, const char *first_prop_name, ...);
GParamSpec *swami_find_object_property(const char *property_name);
GParamSpec **swami_list_object_properties(guint *n_properties);

guint swami_object_get_flags(GObject *object);
void swami_object_set_flags(GObject *object, guint flags);
void swami_object_clear_flags(GObject *object, guint flags);

void swami_object_set_origin(GObject *obj, GObject *origin);
GObject *swami_object_get_origin(GObject *obj);

#endif
