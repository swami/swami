/*
 builtin_enums.c

 This file contains functions registration of type enumeration (or flags) defined in the
 respectives headers files.
 Typically this file could be generated automatically at make time (with the help
 of glib-mkenums).
 Compiling on Windows: glib-mkenums is a perl script and Perl isn't natively installed.
 To avoid Perl dependency, the file should manually updated. This shouldn't be a problem
 when new enumerations are slowly added over time.

 Please respect the naming conventions. This example assumes  an enum definition
 in a header file:
 typedef enum
 {
  SWAMI_CONTROL_SENDS = 1 << 4,
  ....
  ....
 } SwamiControlFlags;

 The name of registration function should be: xxx_get_type, with xxx the enum name (e.g
 word-separated by underscores. (e.g  patch_base_flags_get_type
 When the enum value definitions contain bit-shift operators, this function must call
 g_flags_register_static()otherwise g_enum_register_static() must be called.

 First parameter of g_flags_register_static or g_enum_register_static(type name) must
 be the enum name (e.g "SwamiControlFlags").
 Second parameter of of g_flags_register_static() must be GFlagsValue table value.
 Second parameter of of g_enum_register_static() must be GFEnumValue table value.
 Each value (GFlagsValue or GFEnumValue) must be {ENUM_VALUE, "VALUE_NAME", "valuenick"}:
 - ENUM_VALUE, the integer value for the enum value.(e.g SWAMI_CONTROL_SENDS).
 - VALUE_NAME, name with words uppercase and word-separated by underscores
  (e.g "SWAMI_CONTROL_SENDS").
 - valuenick, this is usually stripping common prefix words of all the enum values.
 the words are lowercase and underscores are substituted by a minus (e.g. "sends").
*/

#include "libswami.h"
#include "swami_priv.h"

/* enumerations from "SwamiControl.h" */
GType
swami_control_flags_get_type(void)
{
    static GType etype = 0;

    if(etype == 0)
    {
        static const GFlagsValue values[] =
        {
            { SWAMI_CONTROL_SENDS, "SWAMI_CONTROL_SENDS", "sends" },
            { SWAMI_CONTROL_RECVS, "SWAMI_CONTROL_RECVS", "recvs" },
            { SWAMI_CONTROL_NO_CONV, "SWAMI_CONTROL_NO_CONV", "no-conv" },
            { SWAMI_CONTROL_NATIVE, "SWAMI_CONTROL_NATIVE", "native" },
            { SWAMI_CONTROL_VALUE, "SWAMI_CONTROL_VALUE", "value" },
            { SWAMI_CONTROL_SPEC_NO_CONV, "SWAMI_CONTROL_SPEC_NO_CONV", "spec-no-conv" },
            { 0, NULL, NULL }
        };
        etype = g_flags_register_static("SwamiControlFlags", values);
    }

    return etype;
}
GType
swami_control_conn_priority_get_type(void)
{
    static GType etype = 0;

    if(etype == 0)
    {
        static const GEnumValue values[] =
        {
            { SWAMI_CONTROL_CONN_PRIORITY_DEFAULT, "SWAMI_CONTROL_CONN_PRIORITY_DEFAULT", "default" },
            { SWAMI_CONTROL_CONN_PRIORITY_LOW, "SWAMI_CONTROL_CONN_PRIORITY_LOW", "low" },
            { SWAMI_CONTROL_CONN_PRIORITY_MEDIUM, "SWAMI_CONTROL_CONN_PRIORITY_MEDIUM", "medium" },
            { SWAMI_CONTROL_CONN_PRIORITY_HIGH, "SWAMI_CONTROL_CONN_PRIORITY_HIGH", "high" },
            { 0, NULL, NULL }
        };
        etype = g_enum_register_static("SwamiControlConnPriority", values);
    }

    return etype;
}
GType
swami_control_conn_flags_get_type(void)
{
    static GType etype = 0;

    if(etype == 0)
    {
        static const GFlagsValue values[] =
        {
            { SWAMI_CONTROL_CONN_INPUT, "SWAMI_CONTROL_CONN_INPUT", "input" },
            { SWAMI_CONTROL_CONN_OUTPUT, "SWAMI_CONTROL_CONN_OUTPUT", "output" },
            { SWAMI_CONTROL_CONN_INIT, "SWAMI_CONTROL_CONN_INIT", "init" },
            { SWAMI_CONTROL_CONN_BIDIR, "SWAMI_CONTROL_CONN_BIDIR", "bidir" },
            { SWAMI_CONTROL_CONN_SPEC, "SWAMI_CONTROL_CONN_SPEC", "spec" },
            { 0, NULL, NULL }
        };
        etype = g_flags_register_static("SwamiControlConnFlags", values);
    }

    return etype;
}

/* enumerations from "SwamiLog.h" */
GType
swami_error_get_type(void)
{
    static GType etype = 0;

    if(etype == 0)
    {
        static const GEnumValue values[] =
        {
            { SWAMI_ERROR_FAIL, "SWAMI_ERROR_FAIL", "fail" },
            { SWAMI_ERROR_INVALID, "SWAMI_ERROR_INVALID", "invalid" },
            { SWAMI_ERROR_CANCELED, "SWAMI_ERROR_CANCELED", "canceled" },
            { SWAMI_ERROR_UNSUPPORTED, "SWAMI_ERROR_UNSUPPORTED", "unsupported" },
            { SWAMI_ERROR_IO, "SWAMI_ERROR_IO", "io" },
            { 0, NULL, NULL }
        };
        etype = g_enum_register_static("SwamiError", values);
    }

    return etype;
}

/* enumerations from "SwamiMidiEvent.h" */
GType
swami_midi_event_type_get_type(void)
{
    static GType etype = 0;

    if(etype == 0)
    {
        static const GEnumValue values[] =
        {
            { SWAMI_MIDI_NONE, "SWAMI_MIDI_NONE", "none" },
            { SWAMI_MIDI_NOTE, "SWAMI_MIDI_NOTE", "note" },
            { SWAMI_MIDI_NOTE_ON, "SWAMI_MIDI_NOTE_ON", "note-on" },
            { SWAMI_MIDI_NOTE_OFF, "SWAMI_MIDI_NOTE_OFF", "note-off" },
            { SWAMI_MIDI_KEY_PRESSURE, "SWAMI_MIDI_KEY_PRESSURE", "key-pressure" },
            { SWAMI_MIDI_PITCH_BEND, "SWAMI_MIDI_PITCH_BEND", "pitch-bend" },
            { SWAMI_MIDI_PROGRAM_CHANGE, "SWAMI_MIDI_PROGRAM_CHANGE", "program-change" },
            { SWAMI_MIDI_CONTROL, "SWAMI_MIDI_CONTROL", "control" },
            { SWAMI_MIDI_CONTROL14, "SWAMI_MIDI_CONTROL14", "control14" },
            { SWAMI_MIDI_CHAN_PRESSURE, "SWAMI_MIDI_CHAN_PRESSURE", "chan-pressure" },
            { SWAMI_MIDI_RPN, "SWAMI_MIDI_RPN", "rpn" },
            { SWAMI_MIDI_NRPN, "SWAMI_MIDI_NRPN", "nrpn" },
            { SWAMI_MIDI_BEND_RANGE, "SWAMI_MIDI_BEND_RANGE", "bend-range" },
            { SWAMI_MIDI_BANK_SELECT, "SWAMI_MIDI_BANK_SELECT", "bank-select" },
            { 0, NULL, NULL }
        };
        etype = g_enum_register_static("SwamiMidiEventType", values);
    }

    return etype;
}

/* enumerations from "SwamiObject.h" */
GType
swami_rank_get_type(void)
{
    static GType etype = 0;

    if(etype == 0)
    {
        static const GEnumValue values[] =
        {
            { SWAMI_RANK_INVALID, "SWAMI_RANK_INVALID", "invalid" },
            { SWAMI_RANK_LOWEST, "SWAMI_RANK_LOWEST", "lowest" },
            { SWAMI_RANK_LOW, "SWAMI_RANK_LOW", "low" },
            { SWAMI_RANK_NORMAL, "SWAMI_RANK_NORMAL", "normal" },
            { SWAMI_RANK_DEFAULT, "SWAMI_RANK_DEFAULT", "default" },
            { SWAMI_RANK_HIGH, "SWAMI_RANK_HIGH", "high" },
            { SWAMI_RANK_HIGHEST, "SWAMI_RANK_HIGHEST", "highest" },
            { 0, NULL, NULL }
        };
        etype = g_enum_register_static("SwamiRank", values);
    }

    return etype;
}
GType
swami_object_flags_get_type(void)
{
    static GType etype = 0;

    if(etype == 0)
    {
        static const GFlagsValue values[] =
        {
            { SWAMI_OBJECT_SAVE, "SWAMI_OBJECT_SAVE", "save" },
            { SWAMI_OBJECT_USER, "SWAMI_OBJECT_USER", "user" },
            { 0, NULL, NULL }
        };
        etype = g_flags_register_static("SwamiObjectFlags", values);
    }

    return etype;
}

/* Generated data ends here */

