/*
 * icons.h - Swami stock icons
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
#ifndef __SWAMIGUI_ICONS_H__
#define __SWAMIGUI_ICONS_H__

/* !! keep synchronized with icons.c !! */
#define SWAMIGUI_STOCK_CONCAVE_NEG_BI	"swamigui_concave_neg_bi"
#define SWAMIGUI_STOCK_CONCAVE_NEG_UNI	"swamigui_concave_neg_uni"
#define SWAMIGUI_STOCK_CONCAVE_POS_BI	"swamigui_concave_pos_bi"
#define SWAMIGUI_STOCK_CONCAVE_POS_UNI	"swamigui_concave_pos_uni"
#define SWAMIGUI_STOCK_CONVEX_NEG_BI	"swamigui_convex_neg_bi"
#define SWAMIGUI_STOCK_CONVEX_NEG_UNI	"swamigui_convex_neg_uni"
#define SWAMIGUI_STOCK_CONVEX_POS_BI	"swamigui_convex_pos_bi"
#define SWAMIGUI_STOCK_CONVEX_POS_UNI	"swamigui_convex_pos_uni"
#define SWAMIGUI_STOCK_DLS		"swamigui_DLS"
#define SWAMIGUI_STOCK_EFFECT_CONTROL	"swamigui_effect_control"
#define SWAMIGUI_STOCK_EFFECT_DEFAULT	"swamigui_effect_default"
#define SWAMIGUI_STOCK_EFFECT_GRAPH	"swamigui_effect_graph"
#define SWAMIGUI_STOCK_EFFECT_SET	"swamigui_effect_set"
#define SWAMIGUI_STOCK_EFFECT_VIEW	"swamigui_effect_view"
#define SWAMIGUI_STOCK_GIG		"swamigui_GIG"
#define SWAMIGUI_STOCK_GLOBAL_ZONE	"swamigui_global_zone"
#define SWAMIGUI_STOCK_INST		"swamigui_inst"
#define SWAMIGUI_STOCK_LINEAR_NEG_BI	"swamigui_linear_neg_bi"
#define SWAMIGUI_STOCK_LINEAR_NEG_UNI	"swamigui_linear_neg_uni"
#define SWAMIGUI_STOCK_LINEAR_POS_BI	"swamigui_linear_pos_bi"
#define SWAMIGUI_STOCK_LINEAR_POS_UNI	"swamigui_linear_pos_uni"
#define SWAMIGUI_STOCK_LOOP_NONE	"swamigui_loop_none"
#define SWAMIGUI_STOCK_LOOP_STANDARD	"swamigui_loop_standard"
#define SWAMIGUI_STOCK_LOOP_RELEASE	"swamigui_loop_release"
#define SWAMIGUI_STOCK_MODENV   	"swamigui_modenv"
#define SWAMIGUI_STOCK_MODENV_ATTACK    "swamigui_modenv_attack"
#define SWAMIGUI_STOCK_MODENV_DECAY     "swamigui_modenv_decay"
#define SWAMIGUI_STOCK_MODENV_DELAY     "swamigui_modenv_delay"
#define SWAMIGUI_STOCK_MODENV_HOLD      "swamigui_modenv_hold"
#define SWAMIGUI_STOCK_MODENV_RELEASE   "swamigui_modenv_release"
#define SWAMIGUI_STOCK_MODENV_SUSTAIN   "swamigui_modenv_sustain"
#define SWAMIGUI_STOCK_MODULATOR_EDITOR	"swamigui_modulator_editor"
#define SWAMIGUI_STOCK_MODULATOR_JUNCT	"swamigui_modulator_junct"
#define SWAMIGUI_STOCK_MUTE		"swamigui_mute"
#define SWAMIGUI_STOCK_PIANO		"swamigui_piano"
#define SWAMIGUI_STOCK_PRESET		"swamigui_preset"
#define SWAMIGUI_STOCK_PYTHON		"swamigui_python"
#define SWAMIGUI_STOCK_SAMPLE		"swamigui_sample"
#define SWAMIGUI_STOCK_SAMPLE_ROM	"swamigui_sample_rom"
#define SWAMIGUI_STOCK_SAMPLE_VIEWER	"swamigui_sample_viewer"
#define SWAMIGUI_STOCK_SOUNDFONT	"swamigui_SoundFont"
#define SWAMIGUI_STOCK_SPLITS		"swamigui_splits"
#define SWAMIGUI_STOCK_SWITCH_NEG_BI	"swamigui_switch_neg_bi"
#define SWAMIGUI_STOCK_SWITCH_NEG_UNI	"swamigui_switch_neg_uni"
#define SWAMIGUI_STOCK_SWITCH_POS_BI	"swamigui_switch_pos_bi"
#define SWAMIGUI_STOCK_SWITCH_POS_UNI	"swamigui_switch_pos_uni"
#define SWAMIGUI_STOCK_TREE		"swamigui_tree"
#define SWAMIGUI_STOCK_TUNING		"swamigui_tuning"
#define SWAMIGUI_STOCK_VELOCITY		"swamigui_velocity"
#define SWAMIGUI_STOCK_VOLENV   	"swamigui_volenv"
#define SWAMIGUI_STOCK_VOLENV_ATTACK    "swamigui_volenv_attack"
#define SWAMIGUI_STOCK_VOLENV_DECAY     "swamigui_volenv_decay"
#define SWAMIGUI_STOCK_VOLENV_DELAY     "swamigui_volenv_delay"
#define SWAMIGUI_STOCK_VOLENV_HOLD      "swamigui_volenv_hold"
#define SWAMIGUI_STOCK_VOLENV_RELEASE   "swamigui_volenv_release"
#define SWAMIGUI_STOCK_VOLENV_SUSTAIN   "swamigui_volenv_sustain"


#define SWAMIGUI_ICON_SIZE_CUSTOM_LARGE1 swamigui_icon_size_custom_large1

extern int swamigui_icon_size_custom_large1;

char *swamigui_icon_get_category_icon (int category);

#endif
