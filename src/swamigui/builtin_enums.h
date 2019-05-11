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

#ifndef __SWAMIGUI_BUILTIN_ENUMS_H__
#define __SWAMIGUI_BUILTIN_ENUMS_H__

#include <glib-object.h>

G_BEGIN_DECLS
/* enumerations from "SwamiguiBarPtr.h" */
GType swamigui_bar_ptr_type_get_type (void);
#define SWAMIGUI_TYPE_BAR_PTR_TYPE (swamigui_bar_ptr_type_get_type())
/* enumerations from "SwamiguiCanvasMod.h" */
GType swamigui_canvas_mod_type_get_type (void);
#define SWAMIGUI_TYPE_CANVAS_MOD_TYPE (swamigui_canvas_mod_type_get_type())
GType swamigui_canvas_mod_axis_get_type (void);
#define SWAMIGUI_TYPE_CANVAS_MOD_AXIS (swamigui_canvas_mod_axis_get_type())
GType swamigui_canvas_mod_flags_get_type (void);
#define SWAMIGUI_TYPE_CANVAS_MOD_FLAGS (swamigui_canvas_mod_flags_get_type())
GType swamigui_canvas_mod_actions_get_type (void);
#define SWAMIGUI_TYPE_CANVAS_MOD_ACTIONS (swamigui_canvas_mod_actions_get_type())
/* enumerations from "SwamiguiControl.h" */
GType swamigui_control_rank_get_type (void);
#define SWAMIGUI_TYPE_CONTROL_RANK (swamigui_control_rank_get_type())
GType swamigui_control_flags_get_type (void);
#define SWAMIGUI_TYPE_CONTROL_FLAGS (swamigui_control_flags_get_type())
GType swamigui_control_object_flags_get_type (void);
#define SWAMIGUI_TYPE_CONTROL_OBJECT_FLAGS (swamigui_control_object_flags_get_type())
/* enumerations from "SwamiguiItemMenu.h" */
GType swamigui_item_menu_flags_get_type (void);
#define SWAMIGUI_TYPE_ITEM_MENU_FLAGS (swamigui_item_menu_flags_get_type())
/* enumerations from "SwamiguiMultiSave.h" */
GType swamigui_multi_save_flags_get_type (void);
#define SWAMIGUI_TYPE_MULTI_SAVE_FLAGS (swamigui_multi_save_flags_get_type())
/* enumerations from "SwamiguiPanelSF2Gen.h" */
GType swamigui_panel_sf2_gen_op_get_type (void);
#define SWAMIGUI_TYPE_PANEL_SF2_GEN_OP (swamigui_panel_sf2_gen_op_get_type())
/* enumerations from "SwamiguiPaste.h" */
GType swamigui_paste_status_get_type (void);
#define SWAMIGUI_TYPE_PASTE_STATUS (swamigui_paste_status_get_type())
GType swamigui_paste_decision_get_type (void);
#define SWAMIGUI_TYPE_PASTE_DECISION (swamigui_paste_decision_get_type())
/* enumerations from "SwamiguiRoot.h" */
GType swamigui_quit_confirm_get_type (void);
#define SWAMIGUI_TYPE_QUIT_CONFIRM (swamigui_quit_confirm_get_type())
/* enumerations from "SwamiguiSampleEditor.h" */
GType swamigui_sample_editor_status_get_type (void);
#define SWAMIGUI_TYPE_SAMPLE_EDITOR_STATUS (swamigui_sample_editor_status_get_type())
GType swamigui_sample_editor_marker_flags_get_type (void);
#define SWAMIGUI_TYPE_SAMPLE_EDITOR_MARKER_FLAGS (swamigui_sample_editor_marker_flags_get_type())
GType swamigui_sample_editor_marker_id_get_type (void);
#define SWAMIGUI_TYPE_SAMPLE_EDITOR_MARKER_ID (swamigui_sample_editor_marker_id_get_type())
/* enumerations from "SwamiguiSplits.h" */
GType swamigui_splits_mode_get_type (void);
#define SWAMIGUI_TYPE_SPLITS_MODE (swamigui_splits_mode_get_type())
GType swamigui_splits_status_get_type (void);
#define SWAMIGUI_TYPE_SPLITS_STATUS (swamigui_splits_status_get_type())
/* enumerations from "SwamiguiStatusbar.h" */
GType swamigui_statusbar_pos_get_type (void);
#define SWAMIGUI_TYPE_STATUSBAR_POS (swamigui_statusbar_pos_get_type())
GType swamigui_statusbar_timeout_get_type (void);
#define SWAMIGUI_TYPE_STATUSBAR_TIMEOUT (swamigui_statusbar_timeout_get_type())
G_END_DECLS

#endif /* __SWAMIGUI_BUILTIN_ENUMS_H__ */

/* Generated data ends here */

