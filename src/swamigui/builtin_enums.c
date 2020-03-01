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
  SWAMIGUI_BAR_PTR_POSITION,
  SWAMIGUI_BAR_PTR_RANGE
 } SwamiguiBarPtrType;


 The name of registration function should be: xxx_get_type, with xxx the enum name (e.g
 word-separated by underscores. (e.g  swamigui_bar_ptr_type_get_type
 When the enum value definitions contain bit-shift operators, this function must call
 g_flags_register_static()otherwise g_enum_register_static() must be called.

 First parameter of g_flags_register_static or g_enum_register_static(type name) must
 be the enum name (e.g "SwamiguiBarPtrType").
 Second parameter of of g_flags_register_static() must be GFlagsValue table value.
 Second parameter of of g_enum_register_static() must be GFEnumValue table value.
 Each value (GFlagsValue or GFEnumValue) must be {ENUM_VALUE, "VALUE_NAME", "valuenick"}:
 - ENUM_VALUE, the integer value for the enum value.(e.g SWAMIGUI_BAR_PTR_POSITION).
 - VALUE_NAME, name with words uppercase and word-separated by underscores
  (e.g "SWAMIGUI_BAR_PTR_POSITION").
 - valuenick, this is usually stripping common prefix words of all the enum values.
 the words are lowercase and underscores are substituted by a minus (e.g. "position").
*/

#include <swamigui/swamigui.h>

/* enumerations from "SwamiguiBarPtr.h" */
GType
swamigui_bar_ptr_type_get_type(void)
{
    static GType etype = 0;

    if(etype == 0)
    {
        static const GEnumValue values[] =
        {
            { SWAMIGUI_BAR_PTR_POSITION, "SWAMIGUI_BAR_PTR_POSITION", "position" },
            { SWAMIGUI_BAR_PTR_RANGE, "SWAMIGUI_BAR_PTR_RANGE", "range" },
            { 0, NULL, NULL }
        };
        etype = g_enum_register_static("SwamiguiBarPtrType", values);
    }

    return etype;
}

/* enumerations from "SwamiguiCanvasMod.h" */
GType
swamigui_canvas_mod_type_get_type(void)
{
    static GType etype = 0;

    if(etype == 0)
    {
        static const GEnumValue values[] =
        {
            { SWAMIGUI_CANVAS_MOD_SNAP_ZOOM, "SWAMIGUI_CANVAS_MOD_SNAP_ZOOM", "snap-zoom" },
            { SWAMIGUI_CANVAS_MOD_WHEEL_ZOOM, "SWAMIGUI_CANVAS_MOD_WHEEL_ZOOM", "wheel-zoom" },
            { SWAMIGUI_CANVAS_MOD_SNAP_SCROLL, "SWAMIGUI_CANVAS_MOD_SNAP_SCROLL", "snap-scroll" },
            { SWAMIGUI_CANVAS_MOD_WHEEL_SCROLL, "SWAMIGUI_CANVAS_MOD_WHEEL_SCROLL", "wheel-scroll" },
            { 0, NULL, NULL }
        };
        etype = g_enum_register_static("SwamiguiCanvasModType", values);
    }

    return etype;
}
GType
swamigui_canvas_mod_axis_get_type(void)
{
    static GType etype = 0;

    if(etype == 0)
    {
        static const GEnumValue values[] =
        {
            { SWAMIGUI_CANVAS_MOD_X, "SWAMIGUI_CANVAS_MOD_X", "x" },
            { SWAMIGUI_CANVAS_MOD_Y, "SWAMIGUI_CANVAS_MOD_Y", "y" },
            { 0, NULL, NULL }
        };
        etype = g_enum_register_static("SwamiguiCanvasModAxis", values);
    }

    return etype;
}
GType
swamigui_canvas_mod_flags_get_type(void)
{
    static GType etype = 0;

    if(etype == 0)
    {
        static const GFlagsValue values[] =
        {
            { SWAMIGUI_CANVAS_MOD_ENABLED, "SWAMIGUI_CANVAS_MOD_ENABLED", "enabled" },
            { 0, NULL, NULL }
        };
        etype = g_flags_register_static("SwamiguiCanvasModFlags", values);
    }

    return etype;
}
GType
swamigui_canvas_mod_actions_get_type(void)
{
    static GType etype = 0;

    if(etype == 0)
    {
        static const GFlagsValue values[] =
        {
            { SWAMIGUI_CANVAS_MOD_ZOOM_X, "SWAMIGUI_CANVAS_MOD_ZOOM_X", "zoom-x" },
            { SWAMIGUI_CANVAS_MOD_ZOOM_Y, "SWAMIGUI_CANVAS_MOD_ZOOM_Y", "zoom-y" },
            { SWAMIGUI_CANVAS_MOD_SCROLL_X, "SWAMIGUI_CANVAS_MOD_SCROLL_X", "scroll-x" },
            { SWAMIGUI_CANVAS_MOD_SCROLL_Y, "SWAMIGUI_CANVAS_MOD_SCROLL_Y", "scroll-y" },
            { 0, NULL, NULL }
        };
        etype = g_flags_register_static("SwamiguiCanvasModActions", values);
    }

    return etype;
}

/* enumerations from "SwamiguiControl.h" */
GType
swamigui_control_rank_get_type(void)
{
    static GType etype = 0;

    if(etype == 0)
    {
        static const GEnumValue values[] =
        {
            { SWAMIGUI_CONTROL_RANK_LOWEST, "SWAMIGUI_CONTROL_RANK_LOWEST", "lowest" },
            { SWAMIGUI_CONTROL_RANK_LOW, "SWAMIGUI_CONTROL_RANK_LOW", "low" },
            { SWAMIGUI_CONTROL_RANK_NORMAL, "SWAMIGUI_CONTROL_RANK_NORMAL", "normal" },
            { SWAMIGUI_CONTROL_RANK_HIGH, "SWAMIGUI_CONTROL_RANK_HIGH", "high" },
            { SWAMIGUI_CONTROL_RANK_HIGHEST, "SWAMIGUI_CONTROL_RANK_HIGHEST", "highest" },
            { 0, NULL, NULL }
        };
        etype = g_enum_register_static("SwamiguiControlRank", values);
    }

    return etype;
}
GType
swamigui_control_flags_get_type(void)
{
    static GType etype = 0;

    if(etype == 0)
    {
        static const GEnumValue values[] =
        {
            { SWAMIGUI_CONTROL_CTRL, "SWAMIGUI_CONTROL_CTRL", "ctrl" },
            { SWAMIGUI_CONTROL_VIEW, "SWAMIGUI_CONTROL_VIEW", "view" },
            { SWAMIGUI_CONTROL_NO_CREATE, "SWAMIGUI_CONTROL_NO_CREATE", "no-create" },
            { 0, NULL, NULL }
        };
        etype = g_enum_register_static("SwamiguiControlFlags", values);
    }

    return etype;
}
GType
swamigui_control_object_flags_get_type(void)
{
    static GType etype = 0;

    if(etype == 0)
    {
        static const GFlagsValue values[] =
        {
            { SWAMIGUI_CONTROL_OBJECT_NO_LABELS, "SWAMIGUI_CONTROL_OBJECT_NO_LABELS", "no-labels" },
            { SWAMIGUI_CONTROL_OBJECT_NO_SORT, "SWAMIGUI_CONTROL_OBJECT_NO_SORT", "no-sort" },
            { SWAMIGUI_CONTROL_OBJECT_PROP_LABELS, "SWAMIGUI_CONTROL_OBJECT_PROP_LABELS", "prop-labels" },
            { 0, NULL, NULL }
        };
        etype = g_flags_register_static("SwamiguiControlObjectFlags", values);
    }

    return etype;
}

/* enumerations from "SwamiguiItemMenu.h" */
GType
swamigui_item_menu_flags_get_type(void)
{
    static GType etype = 0;

    if(etype == 0)
    {
        static const GFlagsValue values[] =
        {
            { SWAMIGUI_ITEM_MENU_INACTIVE, "SWAMIGUI_ITEM_MENU_INACTIVE", "inactive" },
            { SWAMIGUI_ITEM_MENU_PLUGIN, "SWAMIGUI_ITEM_MENU_PLUGIN", "plugin" },
            { 0, NULL, NULL }
        };
        etype = g_flags_register_static("SwamiguiItemMenuFlags", values);
    }

    return etype;
}

/* enumerations from "SwamiguiMultiSave.h" */
GType
swamigui_multi_save_flags_get_type(void)
{
    static GType etype = 0;

    if(etype == 0)
    {
        static const GFlagsValue values[] =
        {
            { SWAMIGUI_MULTI_SAVE_CLOSE_MODE, "SWAMIGUI_MULTI_SAVE_CLOSE_MODE", "mode" },
            { 0, NULL, NULL }
        };
        etype = g_flags_register_static("SwamiguiMultiSaveFlags", values);
    }

    return etype;
}

/* enumerations from "SwamiguiPanelSF2Gen.h" */
GType
swamigui_panel_sf2_gen_op_get_type(void)
{
    static GType etype = 0;

    if(etype == 0)
    {
        static const GEnumValue values[] =
        {
            { SWAMIGUI_PANEL_SF2_GEN_LABEL, "SWAMIGUI_PANEL_SF2_GEN_LABEL", "label" },
            { SWAMIGUI_PANEL_SF2_GEN_COLUMN, "SWAMIGUI_PANEL_SF2_GEN_COLUMN", "column" },
            { SWAMIGUI_PANEL_SF2_GEN_END, "SWAMIGUI_PANEL_SF2_GEN_END", "end" },
            { 0, NULL, NULL }
        };
        etype = g_enum_register_static("SwamiguiPanelSF2GenOp", values);
    }

    return etype;
}

/* enumerations from "SwamiguiPaste.h" */
GType
swamigui_paste_status_get_type(void)
{
    static GType etype = 0;

    if(etype == 0)
    {
        static const GEnumValue values[] =
        {
            { SWAMIGUI_PASTE_NORMAL, "SWAMIGUI_PASTE_NORMAL", "normal" },
            { SWAMIGUI_PASTE_ERROR, "SWAMIGUI_PASTE_ERROR", "error" },
            { SWAMIGUI_PASTE_UNHANDLED, "SWAMIGUI_PASTE_UNHANDLED", "unhandled" },
            { SWAMIGUI_PASTE_CONFLICT, "SWAMIGUI_PASTE_CONFLICT", "conflict" },
            { SWAMIGUI_PASTE_CANCEL, "SWAMIGUI_PASTE_CANCEL", "cancel" },
            { 0, NULL, NULL }
        };
        etype = g_enum_register_static("SwamiguiPasteStatus", values);
    }

    return etype;
}
GType
swamigui_paste_decision_get_type(void)
{
    static GType etype = 0;

    if(etype == 0)
    {
        static const GFlagsValue values[] =
        {
            { SWAMIGUI_PASTE_NO_DECISION, "SWAMIGUI_PASTE_NO_DECISION", "no-decision" },
            { SWAMIGUI_PASTE_SKIP, "SWAMIGUI_PASTE_SKIP", "skip" },
            { SWAMIGUI_PASTE_CHANGED, "SWAMIGUI_PASTE_CHANGED", "changed" },
            { SWAMIGUI_PASTE_REPLACE, "SWAMIGUI_PASTE_REPLACE", "replace" },
            { 0, NULL, NULL }
        };
        etype = g_flags_register_static("SwamiguiPasteDecision", values);
    }

    return etype;
}

/* enumerations from "SwamiguiRoot.h" */
GType
swamigui_quit_confirm_get_type(void)
{
    static GType etype = 0;

    if(etype == 0)
    {
        static const GEnumValue values[] =
        {
            { SWAMIGUI_QUIT_CONFIRM_ALWAYS, "SWAMIGUI_QUIT_CONFIRM_ALWAYS", "always" },
            { SWAMIGUI_QUIT_CONFIRM_UNSAVED, "SWAMIGUI_QUIT_CONFIRM_UNSAVED", "unsaved" },
            { SWAMIGUI_QUIT_CONFIRM_NEVER, "SWAMIGUI_QUIT_CONFIRM_NEVER", "never" },
            { 0, NULL, NULL }
        };
        etype = g_enum_register_static("SwamiguiQuitConfirm", values);
    }

    return etype;
}

/* enumerations from "SwamiguiSampleEditor.h" */
GType
swamigui_sample_editor_status_get_type(void)
{
    static GType etype = 0;

    if(etype == 0)
    {
        static const GEnumValue values[] =
        {
            { SWAMIGUI_SAMPLE_EDITOR_NORMAL, "SWAMIGUI_SAMPLE_EDITOR_NORMAL", "normal" },
            { SWAMIGUI_SAMPLE_EDITOR_INIT, "SWAMIGUI_SAMPLE_EDITOR_INIT", "init" },
            { SWAMIGUI_SAMPLE_EDITOR_UPDATE, "SWAMIGUI_SAMPLE_EDITOR_UPDATE", "update" },
            { 0, NULL, NULL }
        };
        etype = g_enum_register_static("SwamiguiSampleEditorStatus", values);
    }

    return etype;
}
GType
swamigui_sample_editor_marker_flags_get_type(void)
{
    static GType etype = 0;

    if(etype == 0)
    {
        static const GFlagsValue values[] =
        {
            { SWAMIGUI_SAMPLE_EDITOR_MARKER_SINGLE, "SWAMIGUI_SAMPLE_EDITOR_MARKER_SINGLE", "single" },
            { SWAMIGUI_SAMPLE_EDITOR_MARKER_VIEW, "SWAMIGUI_SAMPLE_EDITOR_MARKER_VIEW", "view" },
            { SWAMIGUI_SAMPLE_EDITOR_MARKER_SIZE, "SWAMIGUI_SAMPLE_EDITOR_MARKER_SIZE", "size" },
            { 0, NULL, NULL }
        };
        etype = g_flags_register_static("SwamiguiSampleEditorMarkerFlags", values);
    }

    return etype;
}
GType
swamigui_sample_editor_marker_id_get_type(void)
{
    static GType etype = 0;

    if(etype == 0)
    {
        static const GEnumValue values[] =
        {
            { SWAMIGUI_SAMPLE_EDITOR_MARKER_ID_SELECTION, "SWAMIGUI_SAMPLE_EDITOR_MARKER_ID_SELECTION", "selection" },
            { SWAMIGUI_SAMPLE_EDITOR_MARKER_ID_LOOP_FIND_START, "SWAMIGUI_SAMPLE_EDITOR_MARKER_ID_LOOP_FIND_START", "loop-find-start" },
            { SWAMIGUI_SAMPLE_EDITOR_MARKER_ID_LOOP_FIND_END, "SWAMIGUI_SAMPLE_EDITOR_MARKER_ID_LOOP_FIND_END", "loop-find-end" },
            { 0, NULL, NULL }
        };
        etype = g_enum_register_static("SwamiguiSampleEditorMarkerId", values);
    }

    return etype;
}

/* enumerations from "SwamiguiSplits.h" */
GType
swamigui_splits_mode_get_type(void)
{
    static GType etype = 0;

    if(etype == 0)
    {
        static const GEnumValue values[] =
        {
            { SWAMIGUI_SPLITS_NOTE, "SWAMIGUI_SPLITS_NOTE", "note" },
            { SWAMIGUI_SPLITS_VELOCITY, "SWAMIGUI_SPLITS_VELOCITY", "velocity" },
            { 0, NULL, NULL }
        };
        etype = g_enum_register_static("SwamiguiSplitsMode", values);
    }

    return etype;
}
GType
swamigui_splits_status_get_type(void)
{
    static GType etype = 0;

    if(etype == 0)
    {
        static const GEnumValue values[] =
        {
            { SWAMIGUI_SPLITS_NORMAL, "SWAMIGUI_SPLITS_NORMAL", "normal" },
            { SWAMIGUI_SPLITS_INIT, "SWAMIGUI_SPLITS_INIT", "init" },
            { SWAMIGUI_SPLITS_MODE, "SWAMIGUI_SPLITS_MODE", "mode" },
            { SWAMIGUI_SPLITS_UPDATE, "SWAMIGUI_SPLITS_UPDATE", "update" },
            { SWAMIGUI_SPLITS_CHANGED, "SWAMIGUI_SPLITS_CHANGED", "changed" },
            { 0, NULL, NULL }
        };
        etype = g_enum_register_static("SwamiguiSplitsStatus", values);
    }

    return etype;
}

/* enumerations from "SwamiguiStatusbar.h" */
GType
swamigui_statusbar_pos_get_type(void)
{
    static GType etype = 0;

    if(etype == 0)
    {
        static const GEnumValue values[] =
        {
            { SWAMIGUI_STATUSBAR_POS_LEFT, "SWAMIGUI_STATUSBAR_POS_LEFT", "left" },
            { SWAMIGUI_STATUSBAR_POS_RIGHT, "SWAMIGUI_STATUSBAR_POS_RIGHT", "right" },
            { 0, NULL, NULL }
        };
        etype = g_enum_register_static("SwamiguiStatusbarPos", values);
    }

    return etype;
}
GType
swamigui_statusbar_timeout_get_type(void)
{
    static GType etype = 0;

    if(etype == 0)
    {
        static const GEnumValue values[] =
        {
            { SWAMIGUI_STATUSBAR_TIMEOUT_DEFAULT, "SWAMIGUI_STATUSBAR_TIMEOUT_DEFAULT", "default" },
            { SWAMIGUI_STATUSBAR_TIMEOUT_FOREVER, "SWAMIGUI_STATUSBAR_TIMEOUT_FOREVER", "forever" },
            { 0, NULL, NULL }
        };
        etype = g_enum_register_static("SwamiguiStatusbarTimeout", values);
    }

    return etype;
}

/* Generated data ends here */

