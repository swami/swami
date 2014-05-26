/*
 * libswami.h - Main libswami header file that includes all others
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
#ifndef __LIB_SWAMI_H__
#define __LIB_SWAMI_H__

#include <glib-object.h>
#include <libinstpatch/IpatchItem.h>

#include <libswami/SwamiControl.h>
#include <libswami/SwamiEvent_ipatch.h>

/* libswami.c */
void swami_init (void);

/* IpatchItem event controls */
extern SwamiControl *swami_patch_prop_title_control;
extern SwamiControl *swami_patch_add_control;
extern SwamiControl *swami_patch_remove_control;

#include <libswami/SwamiControlEvent.h>
#include <libswami/SwamiControlFunc.h>
#include <libswami/SwamiControlHub.h>
#include <libswami/SwamiControlMidi.h>
#include <libswami/SwamiControlProp.h>
#include <libswami/SwamiControlQueue.h>
#include <libswami/SwamiControlValue.h>
#include <libswami/SwamiEvent_ipatch.h>
#include <libswami/SwamiLock.h>
#include <libswami/SwamiLog.h>
#include <libswami/SwamiMidiDevice.h>
#include <libswami/SwamiMidiEvent.h>
#include <libswami/SwamiObject.h>
#include <libswami/SwamiParam.h>
#include <libswami/SwamiPlugin.h>
#include <libswami/SwamiPropTree.h>
#include <libswami/SwamiRoot.h>
#include <libswami/SwamiWavetbl.h>
#include <libswami/builtin_enums.h>
#include <libswami/util.h>
#include <libswami/version.h>

#endif
