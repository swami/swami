/*
 * SwamiWavetbl.h - Swami wave table object
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
#ifndef __SWAMI_WAVETBL_H__
#define __SWAMI_WAVETBL_H__

#include <glib.h>
#include <glib-object.h>
#include <libinstpatch/libinstpatch.h>
#include <libswami/SwamiLock.h>
#include <libswami/SwamiMidiDevice.h>
#include <libswami/SwamiControlMidi.h>

typedef struct _SwamiWavetbl SwamiWavetbl;
typedef struct _SwamiWavetblClass SwamiWavetblClass;

#define SWAMI_TYPE_WAVETBL   (swami_wavetbl_get_type ())
#define SWAMI_WAVETBL(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWAMI_TYPE_WAVETBL, SwamiWavetbl))
#define SWAMI_WAVETBL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), SWAMI_TYPE_WAVETBL, SwamiWavetblClass))
#define SWAMI_IS_WAVETBL(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWAMI_TYPE_WAVETBL))
#define SWAMI_IS_WAVETBL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), SWAMI_TYPE_WAVETBL))
#define SWAMI_WAVETBL_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), SWAMI_TYPE_WAVETBL, SwamiWavetblClass))

/* Swami Wavetbl object */
struct _SwamiWavetbl
{
    SwamiLock parent_instance;

    IpatchVBank *vbank;		/* Virtual bank of available instruments */

    /*< private >*/

    gboolean active;		/* driver is active? */
    guint16 active_bank;		/* active (focused) audible MIDI bank number */
    guint16 active_program;	/* active (focused) audible MIDI program number */
};

struct _SwamiWavetblClass
{
    SwamiLockClass parent_class;

    /*< public >*/

    gboolean(*open)(SwamiWavetbl *wavetbl, GError **err);
    void (*close)(SwamiWavetbl *wavetbl);
    SwamiControlMidi *(*get_control)(SwamiWavetbl *wavetbl, int index);
    gboolean(*load_patch)(SwamiWavetbl *wavetbl, IpatchItem *patch,
                          GError **err);
    gboolean(*load_active_item)(SwamiWavetbl *wavetbl, IpatchItem *item,
                                GError **err);
    gboolean(*check_update_item)(SwamiWavetbl *wavetbl, IpatchItem *item,
                                 GParamSpec *prop);
    void (*update_item)(SwamiWavetbl *wavetbl, IpatchItem *item);
    void (*realtime_effect)(SwamiWavetbl *wavetbl, IpatchItem *item,
                            GParamSpec *prop, GValue *value);
};


GType swami_wavetbl_get_type(void);

IpatchVBank *swami_wavetbl_get_virtual_bank(SwamiWavetbl *wavetbl);
void swami_wavetbl_set_active_item_locale(SwamiWavetbl *wavetbl,
        int bank, int program);
void swami_wavetbl_get_active_item_locale(SwamiWavetbl *wavetbl,
        int *bank, int *program);

gboolean swami_wavetbl_open(SwamiWavetbl *wavetbl, GError **err);
void swami_wavetbl_close(SwamiWavetbl *wavetbl);
SwamiControlMidi *swami_wavetbl_get_control(SwamiWavetbl *wavetbl, int index);
gboolean swami_wavetbl_load_patch(SwamiWavetbl *wavetbl, IpatchItem *patch,
                                  GError **err);
gboolean swami_wavetbl_load_active_item(SwamiWavetbl *wavetbl,
                                        IpatchItem *item, GError **err);
gboolean swami_wavetbl_check_update_item(SwamiWavetbl *wavetbl, IpatchItem *item,
        GParamSpec *prop);
void swami_wavetbl_update_item(SwamiWavetbl *wavetbl, IpatchItem *item);

#endif
