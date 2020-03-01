/*
 * SwamiControlHub.h - Header for control hub object
 * Re-transmits any events that it receives
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
#ifndef __SWAMI_CONTROL_HUB_H__
#define __SWAMI_CONTROL_HUB_H__

#include <glib.h>
#include <glib-object.h>
#include <libswami/SwamiControl.h>

typedef struct _SwamiControlHub SwamiControlHub;
typedef struct _SwamiControlHubClass SwamiControlHubClass;

#define SWAMI_TYPE_CONTROL_HUB   (swami_control_hub_get_type ())
#define SWAMI_CONTROL_HUB(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWAMI_TYPE_CONTROL_HUB, \
   SwamiControlHub))
#define SWAMI_CONTROL_HUB_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), SWAMI_TYPE_CONTROL_HUB, \
			    SwamiControlHubClass))
#define SWAMI_IS_CONTROL_HUB(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWAMI_TYPE_CONTROL_HUB))
#define SWAMI_IS_CONTROL_HUB_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), SWAMI_TYPE_CONTROL_HUB))

/* control hub object */
struct _SwamiControlHub
{
    SwamiControl parent_instance;	/* derived from SwamiControl */
};

/* control hub class */
struct _SwamiControlHubClass
{
    SwamiControlClass parent_class;
};

GType swami_control_hub_get_type(void);
SwamiControlHub *swami_control_hub_new(void);

#endif
