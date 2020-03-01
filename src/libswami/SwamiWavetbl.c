/*
 * SwamiWavetbl.c - Swami Wavetable object (base class for drivers)
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
#include <stdio.h>

#include "SwamiWavetbl.h"
#include "SwamiLog.h"
#include "i18n.h"

/* --- signals and properties --- */

enum
{
    ACTIVE,
    LAST_SIGNAL
};

enum
{
    PROP_0,
    PROP_VBANK,
    PROP_ACTIVE,
    PROP_ACTIVE_BANK,
    PROP_ACTIVE_PROGRAM
};

/* --- private function prototypes --- */

static void swami_wavetbl_class_init(SwamiWavetblClass *klass);
static void swami_wavetbl_set_property(GObject *object, guint property_id,
                                       const GValue *value,
                                       GParamSpec *pspec);
static void swami_wavetbl_get_property(GObject *object, guint property_id,
                                       GValue *value, GParamSpec *pspec);
static void swami_wavetbl_init(SwamiWavetbl *wavetbl);
static void swami_wavetbl_finalize(GObject *object);

/* --- private data --- */

SwamiWavetblClass *wavetbl_class = NULL;
static guint wavetbl_signals[LAST_SIGNAL] = { 0 };
static GObjectClass *parent_class = NULL;


/* --- functions --- */

GType
swami_wavetbl_get_type(void)
{
    static GType item_type = 0;

    if(!item_type)
    {
        static const GTypeInfo item_info =
        {
            sizeof(SwamiWavetblClass), NULL, NULL,
            (GClassInitFunc) swami_wavetbl_class_init, NULL, NULL,
            sizeof(SwamiWavetbl), 0,
            (GInstanceInitFunc) swami_wavetbl_init,
        };

        item_type = g_type_register_static(SWAMI_TYPE_LOCK, "SwamiWavetbl",
                                           &item_info, G_TYPE_FLAG_ABSTRACT);
    }

    return (item_type);
}

static void
swami_wavetbl_class_init(SwamiWavetblClass *klass)
{
    GObjectClass *objclass = G_OBJECT_CLASS(klass);

    parent_class = g_type_class_peek_parent(klass);

    objclass->set_property = swami_wavetbl_set_property;
    objclass->get_property = swami_wavetbl_get_property;
    objclass->finalize = swami_wavetbl_finalize;

    wavetbl_signals[ACTIVE] =
        g_signal_new("active", G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_FIRST,
                     0, NULL, NULL,
                     g_cclosure_marshal_VOID__BOOLEAN, G_TYPE_NONE, 1,
                     G_TYPE_BOOLEAN);

    g_object_class_install_property(objclass, PROP_VBANK,
                                    g_param_spec_object("vbank", _("Virtual bank"), _("Virtual bank"),
                                            IPATCH_TYPE_VBANK, G_PARAM_READABLE));
    g_object_class_install_property(objclass, PROP_ACTIVE,
                                    g_param_spec_boolean("active", _("Active"), _("State of driver"),
                                            FALSE, G_PARAM_READABLE));
    g_object_class_install_property(objclass, PROP_ACTIVE_BANK,
                                    g_param_spec_int("active-bank", _("Active bank"),
                                            _("Active (focused) MIDI bank number"),
                                            0, 128, 127,
                                            G_PARAM_READWRITE));
    g_object_class_install_property(objclass, PROP_ACTIVE_PROGRAM,
                                    g_param_spec_int("active-program", _("Active program"),
                                            _("Active (focused) MIDI program number"),
                                            0, 127, 127,
                                            G_PARAM_READWRITE));
}

static void
swami_wavetbl_set_property(GObject *object, guint property_id,
                           const GValue *value, GParamSpec *pspec)
{
    SwamiWavetbl *wavetbl = SWAMI_WAVETBL(object);

    switch(property_id)
    {
    case PROP_ACTIVE_BANK:
        wavetbl->active_bank = g_value_get_int(value);
        break;

    case PROP_ACTIVE_PROGRAM:
        wavetbl->active_program = g_value_get_int(value);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

static void
swami_wavetbl_get_property(GObject *object, guint property_id, GValue *value,
                           GParamSpec *pspec)
{
    SwamiWavetbl *wavetbl;

    g_return_if_fail(SWAMI_IS_WAVETBL(object));

    wavetbl = SWAMI_WAVETBL(object);

    switch(property_id)
    {
    case PROP_VBANK:
        g_value_set_object(value, G_OBJECT(wavetbl->vbank));
        break;

    case PROP_ACTIVE:
        g_value_set_boolean(value, wavetbl->active);
        break;

    case PROP_ACTIVE_BANK:
        g_value_set_int(value, wavetbl->active_bank);
        break;

    case PROP_ACTIVE_PROGRAM:
        g_value_set_int(value, wavetbl->active_program);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

static void
swami_wavetbl_init(SwamiWavetbl *wavetbl)
{
    wavetbl->vbank = ipatch_vbank_new();
    wavetbl->active = FALSE;
    wavetbl->active_bank = 127;
    wavetbl->active_program = 127;
}

static void
swami_wavetbl_finalize(GObject *object)
{
    SwamiWavetbl *wavetbl = SWAMI_WAVETBL(object);

    swami_wavetbl_close(wavetbl);

    g_object_unref(wavetbl->vbank);

    if(parent_class->finalize)
    {
        parent_class->finalize(object);
    }
}

/**
 * swami_wavetbl_get_virtual_bank:
 * @wavetbl: Swami Wavetable object
 *
 * Retrieve the #IpatchVBank object from a Wavetable instance.  This
 * bank is the main synthesis object for the Wavetable instance, which is used
 * for mapping instruments to MIDI bank:program locales.
 *
 * Returns: The virtual bank of the Wavetable instance with a reference count
 *   added for the caller.
 */
IpatchVBank *
swami_wavetbl_get_virtual_bank(SwamiWavetbl *wavetbl)
{
    return (g_object_ref(wavetbl->vbank));
}

/**
 * swami_wavetbl_set_active_item_locale:
 * @wavetbl: Swami wave table object
 * @bank: MIDI bank number of active item
 * @program: MIDI program number of active item
 *
 * Sets the MIDI bank and program numbers (MIDI locale) of the
 * active item. The active item is the currently focused item in the user
 * interface, which doesn't necessarily have its own locale bank and program.
 *
 * MT-NOTE: This function ensures bank and program number are set atomically,
 * which is not assured if using the standard
 * <function>g_object_set()</function> routine.
 */
void
swami_wavetbl_set_active_item_locale(SwamiWavetbl *wavetbl,
                                     int bank, int program)
{
    g_return_if_fail(SWAMI_IS_WAVETBL(wavetbl));
    g_return_if_fail(bank >= 0 && bank <= 128);
    g_return_if_fail(program >= 0 && program <= 127);

    SWAMI_LOCK_WRITE(wavetbl);
    wavetbl->active_bank = bank;
    wavetbl->active_program = program;
    SWAMI_UNLOCK_WRITE(wavetbl);
}

/**
 * swami_wavetbl_get_active_item_locale:
 * @wavetbl: Swami wave table object
 * @bank: Location to store MIDI bank number of active item or %NULL
 * @program: Location to store MIDI program number of active item or %NULL
 *
 * Gets the MIDI bank and program numbers (MIDI locale) of the active
 * item. See swami_wavetbl_set_active_item_locale() for more info.
 */
void
swami_wavetbl_get_active_item_locale(SwamiWavetbl *wavetbl,
                                     int *bank, int *program)
{
    g_return_if_fail(SWAMI_IS_WAVETBL(wavetbl));

    SWAMI_LOCK_READ(wavetbl);

    if(bank)
    {
        *bank = wavetbl->active_bank;
    }

    if(program)
    {
        *program = wavetbl->active_program;
    }

    SWAMI_UNLOCK_READ(wavetbl);
}

/**
 * swami_wavetbl_open:
 * @wavetbl: Swami Wavetable object
 * @err: Location to store error information or %NULL.
 *
 * Open Wavetbl driver.
 *
 * Returns: %TRUE on success, %FALSE otherwise
 */
gboolean
swami_wavetbl_open(SwamiWavetbl *wavetbl, GError **err)
{
    SwamiWavetblClass *wavetbl_class;
    gboolean retval = TRUE;

    g_return_val_if_fail(SWAMI_IS_WAVETBL(wavetbl), FALSE);
    wavetbl_class = SWAMI_WAVETBL_GET_CLASS(wavetbl);
    g_return_val_if_fail(wavetbl_class->open != NULL, FALSE);

    if(wavetbl->active)
    {
        return (TRUE);
    }

    retval = wavetbl_class->open(wavetbl, err);

    if(retval)
    {
        g_signal_emit(G_OBJECT(wavetbl), wavetbl_signals[ACTIVE], 0, TRUE);
    }

    return (retval);
}

/**
 * swami_wavetbl_close:
 * @wavetbl: Swami Wavetable object
 *
 * Close driver, has no effect if already closed.  Emits the "active" signal.
 */
void
swami_wavetbl_close(SwamiWavetbl *wavetbl)
{
    SwamiWavetblClass *oclass;

    g_return_if_fail(SWAMI_IS_WAVETBL(wavetbl));
    oclass = SWAMI_WAVETBL_GET_CLASS(wavetbl);
    g_return_if_fail(oclass->close != NULL);

    if(!wavetbl->active)
    {
        return;
    }

    (*oclass->close)(wavetbl);

    g_signal_emit(G_OBJECT(wavetbl), wavetbl_signals[ACTIVE], 0, FALSE);
}

/**
 * swami_wavetbl_get_control:
 * @wavetbl: Swami Wavetable object
 * @index: Control index
 *
 * Get a MIDI control from a wavetable object. Calls the wavetable's
 * "get_control" method.  A control @index is used to support multiple
 * controls (for example if the wavetable device supports more than 16
 * MIDI channels).
 *
 * Returns: New MIDI control object linked to @wavetbl or %NULL if no
 * control with the given @index. The returned control's reference count
 * has been incremented and is owned by the caller, remember to unref it
 * when finished.
 */
SwamiControlMidi *
swami_wavetbl_get_control(SwamiWavetbl *wavetbl, int index)
{
    SwamiWavetblClass *wavetbl_class;
    SwamiControlMidi *control = NULL;

    g_return_val_if_fail(SWAMI_IS_WAVETBL(wavetbl), NULL);
    wavetbl_class = SWAMI_WAVETBL_GET_CLASS(wavetbl);

    if(wavetbl_class->get_control)
    {
        control = (*wavetbl_class->get_control)(wavetbl, index);

        if(control)
        {
            g_object_ref(control);    /* ++ ref control for caller */
        }
    }

    return (control);
}

/**
 * swami_wavetbl_load_patch:
 * @wavetbl: Swami Wavetable object
 * @patch: Patch object to load
 * @err: Location to store error information or %NULL.
 *
 * Load a patch into a wavetable object.
 *
 * Returns: %TRUE on success, %FALSE otherwise
 */
gboolean
swami_wavetbl_load_patch(SwamiWavetbl *wavetbl, IpatchItem *patch,
                         GError **err)
{
    SwamiWavetblClass *wavetbl_class;

    g_return_val_if_fail(SWAMI_IS_WAVETBL(wavetbl), FALSE);
    g_return_val_if_fail(IPATCH_IS_ITEM(patch), FALSE);

    wavetbl_class = SWAMI_WAVETBL_GET_CLASS(wavetbl);
    g_return_val_if_fail(wavetbl_class->load_patch != NULL, FALSE);

    return ((*wavetbl_class->load_patch)(wavetbl, patch, err));
}

/**
 * swami_wavetbl_load_active_item:
 * @wavetbl: Swami Wavetable object
 * @item: Patch item to load as active item.
 * @err: Location to store error information or %NULL.
 *
 * Load an item as the active program item.
 *
 * Returns: %TRUE on success, %FALSE otherwise
 */
gboolean
swami_wavetbl_load_active_item(SwamiWavetbl *wavetbl, IpatchItem *item,
                               GError **err)
{
    SwamiWavetblClass *wavetbl_class;
    gboolean retval;

    g_return_val_if_fail(SWAMI_IS_WAVETBL(wavetbl), FALSE);
    g_return_val_if_fail(IPATCH_IS_ITEM(item), FALSE);

    wavetbl_class = SWAMI_WAVETBL_GET_CLASS(wavetbl);
    g_return_val_if_fail(wavetbl_class->load_active_item != NULL, FALSE);

    retval = (*wavetbl_class->load_active_item)(wavetbl, item, err);

    return (retval);
}

/**
 * swami_wavetbl_check_update_item:
 * @wavetbl: Swami Wavetable object
 * @item: Patch item to check if an update is needed
 * @prop: #GParamSpec of property that has changed
 *
 * Checks if a given @item needs to be updated if the property @prop has
 * changed.
 *
 * Returns: %TRUE if @item should be updated, %FALSE otherwise (@prop is not
 * a synthesis property or @item is not currently loaded in @wavetbl).
 */
gboolean
swami_wavetbl_check_update_item(SwamiWavetbl *wavetbl, IpatchItem *item,
                                GParamSpec *prop)
{
    SwamiWavetblClass *oclass;
    gboolean retval;

    g_return_val_if_fail(SWAMI_IS_WAVETBL(wavetbl), FALSE);
    g_return_val_if_fail(IPATCH_IS_ITEM(item), FALSE);
    g_return_val_if_fail(G_IS_PARAM_SPEC(prop), FALSE);

    oclass = SWAMI_WAVETBL_GET_CLASS(wavetbl);

    if(!oclass->update_item)
    {
        return (FALSE);
    }

    retval = (*oclass->check_update_item)(wavetbl, item, prop);

    return (retval);
}

/**
 * swami_wavetbl_update_item:
 * @wavetbl: Swami Wavetable object
 * @item: Patch item to force update on
 *
 * Refresh a given @item object's synthesis cache. This should be called after
 * a change affecting synthesis output occurs to @item, which can be tested
 * with swami_wavetbl_check_update_item().
 */
void
swami_wavetbl_update_item(SwamiWavetbl *wavetbl, IpatchItem *item)
{
    SwamiWavetblClass *oclass;

    g_return_if_fail(SWAMI_IS_WAVETBL(wavetbl));
    g_return_if_fail(IPATCH_IS_ITEM(item));

    oclass = SWAMI_WAVETBL_GET_CLASS(wavetbl);

    if(!oclass->update_item)
    {
        return;
    }

    (*oclass->update_item)(wavetbl, item);
}
