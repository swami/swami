/*
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
#include <stdlib.h>
#include <gtk/gtk.h>

#include "SwamiguiPanelSF2GenMisc.h"
#include "SwamiguiPanel.h"
#include "icons.h"
#include "util.h"
#include "i18n.h"


SwamiguiPanelSF2GenCtrlInfo sf2_gen_misc_ctrl_info[] = {
  { SWAMIGUI_PANEL_SF2_GEN_LABEL, N_("<b>Pitch</b>") },
  { IPATCH_SF2_GEN_COARSE_TUNE, GTK_STOCK_CONNECT },
  { IPATCH_SF2_GEN_FINE_TUNE_OVERRIDE, GTK_STOCK_CONNECT },
  { IPATCH_SF2_GEN_SCALE_TUNE, GTK_STOCK_CONNECT },
  { SWAMIGUI_PANEL_SF2_GEN_LABEL, N_("<b>Effects</b>") },
  { IPATCH_SF2_GEN_FILTER_Q, GTK_STOCK_CONNECT },
  { IPATCH_SF2_GEN_FILTER_CUTOFF, GTK_STOCK_CONNECT },
  { IPATCH_SF2_GEN_REVERB, GTK_STOCK_CONNECT },
  { IPATCH_SF2_GEN_CHORUS, GTK_STOCK_CONNECT },
  { IPATCH_SF2_GEN_PAN, GTK_STOCK_CONNECT },

  { SWAMIGUI_PANEL_SF2_GEN_COLUMN, NULL },

  { SWAMIGUI_PANEL_SF2_GEN_LABEL, N_("<b>Modulation LFO</b>") },
  { IPATCH_SF2_GEN_MOD_LFO_DELAY, GTK_STOCK_CONNECT },
  { IPATCH_SF2_GEN_MOD_LFO_FREQ, GTK_STOCK_CONNECT },
  { IPATCH_SF2_GEN_MOD_LFO_TO_PITCH, GTK_STOCK_CONNECT },
  { IPATCH_SF2_GEN_MOD_LFO_TO_FILTER_CUTOFF, GTK_STOCK_CONNECT },
  { IPATCH_SF2_GEN_MOD_LFO_TO_VOLUME, GTK_STOCK_CONNECT },
  { SWAMIGUI_PANEL_SF2_GEN_LABEL, N_("<b>Vibrato LFO</b>") },
  { IPATCH_SF2_GEN_VIB_LFO_DELAY, GTK_STOCK_CONNECT },
  { IPATCH_SF2_GEN_VIB_LFO_FREQ, GTK_STOCK_CONNECT },
  { IPATCH_SF2_GEN_VIB_LFO_TO_PITCH, GTK_STOCK_CONNECT },

  { SWAMIGUI_PANEL_SF2_GEN_END, NULL }
};

static void swamigui_panel_sf2_gen_misc_panel_iface_init (SwamiguiPanelIface *panel_iface);


G_DEFINE_TYPE_WITH_CODE (SwamiguiPanelSF2GenMisc, swamigui_panel_sf2_gen_misc,
			 SWAMIGUI_TYPE_PANEL_SF2_GEN,
		    G_IMPLEMENT_INTERFACE (SWAMIGUI_TYPE_PANEL,
					   swamigui_panel_sf2_gen_misc_panel_iface_init));

static void
swamigui_panel_sf2_gen_misc_class_init (SwamiguiPanelSF2GenMiscClass *klass)
{
}

static void
swamigui_panel_sf2_gen_misc_panel_iface_init (SwamiguiPanelIface *panel_iface)
{
  panel_iface->label = _("Misc. Controls");
  panel_iface->blurb = _("Tuning, effects and oscillator controls");
  panel_iface->stockid = SWAMIGUI_STOCK_EFFECT_CONTROL;
}

static void
swamigui_panel_sf2_gen_misc_init (SwamiguiPanelSF2GenMisc *genpanel)
{
  swamigui_panel_sf2_gen_set_controls (SWAMIGUI_PANEL_SF2_GEN (genpanel),
                                       sf2_gen_misc_ctrl_info);
}

/**
 * swamigui_panel_sf2_gen_misc_new:
 *
 * Create a new generator control panel object.
 *
 * Returns: new widget of type #SwamiguiPanelSF2GenMisc
 */
GtkWidget *
swamigui_panel_sf2_gen_misc_new (void)
{
  return (GTK_WIDGET (gtk_type_new (swamigui_panel_sf2_gen_misc_get_type ())));
}
