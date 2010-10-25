/*
 * SwamiContainer.h - Root container for instrument patches
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
#ifndef __SWAMI_CONTAINER_H__
#define __SWAMI_CONTAINER_H__

#include <glib.h>
#include <glib-object.h>
#include <libinstpatch/libinstpatch.h>

typedef struct _SwamiContainer SwamiContainer;
typedef struct _SwamiContainerClass SwamiContainerClass;

#include <libswami/SwamiRoot.h>

#define SWAMI_TYPE_CONTAINER   (swami_container_get_type ())
#define SWAMI_CONTAINER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWAMI_TYPE_CONTAINER, SwamiContainer))
#define SWAMI_CONTAINER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), SWAMI_TYPE_CONTAINER, SwamiContainerClass))
#define SWAMI_IS_CONTAINER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWAMI_TYPE_CONTAINER))
#define SWAMI_IS_CONTAINER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), SWAMI_TYPE_CONTAINER))

struct _SwamiContainer
{
  IpatchContainer parent;

  /*< private >*/
  GSList *patch_list;		/* list of instrument patches */
  SwamiRoot *root;		/* root object owning this container */
};

struct _SwamiContainerClass
{
  IpatchContainerClass parent_class;
};

GType swami_container_get_type (void);
SwamiContainer *swami_container_new (void);

#endif
