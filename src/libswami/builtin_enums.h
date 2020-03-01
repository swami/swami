/* builin_enums.h
 Prototype of registration functions of type enumeration (or flags) defined in the
 respectives headers files.
 Typically this file could be generated automatically at make time (with the help
 of glib-mkenums).
 Compiling on Windows: glib-mkenums is a perl script and Perl isn't natively installed.
 To avoid Perl dependency, the file should manually updated. This shouldn't be a problem
 when new enumerations are slowly added over time.

 Please read the naming conventions described in builin_enums.c.
*/

#ifndef __SWAMI_BUILTIN_ENUMS_H__
#define __SWAMI_BUILTIN_ENUMS_H__

#include <glib-object.h>

G_BEGIN_DECLS
/* enumerations from "SwamiControl.h" */
GType swami_control_flags_get_type(void);
#define SWAMI_TYPE_CONTROL_FLAGS (swami_control_flags_get_type())
GType swami_control_conn_priority_get_type(void);
#define SWAMI_TYPE_CONTROL_CONN_PRIORITY (swami_control_conn_priority_get_type())
GType swami_control_conn_flags_get_type(void);
#define SWAMI_TYPE_CONTROL_CONN_FLAGS (swami_control_conn_flags_get_type())
/* enumerations from "SwamiLog.h" */
GType swami_error_get_type(void);
#define SWAMI_TYPE_ERROR (swami_error_get_type())
/* enumerations from "SwamiMidiEvent.h" */
GType swami_midi_event_type_get_type(void);
#define SWAMI_TYPE_MIDI_EVENT_TYPE (swami_midi_event_type_get_type())
/* enumerations from "SwamiObject.h" */
GType swami_rank_get_type(void);
#define SWAMI_TYPE_RANK (swami_rank_get_type())
GType swami_object_flags_get_type(void);
#define SWAMI_TYPE_OBJECT_FLAGS (swami_object_flags_get_type())
G_END_DECLS

#endif /* __SWAMI_BUILTIN_ENUMS_H__ */

/* Generated data ends here */

