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

#include <libinstpatch/libinstpatch.h>
#include <libswami/libswami.h>

#include "SwamiguiControl.h"
#include "SwamiguiPanelSF2GenEnv.h"
#include "SwamiguiPanel.h"
#include "SwamiguiRoot.h"
#include "SwamiguiSpinScale.h"
#include "icons.h"
#include "util.h"
#include "i18n.h"


SwamiguiPanelSF2GenCtrlInfo sf2_gen_env_ctrl_info[] = {
  { SWAMIGUI_PANEL_SF2_GEN_LABEL, N_("<b>Volume Envelope</b>") },
  { IPATCH_SF2_GEN_VOL_ENV_DELAY, SWAMIGUI_STOCK_VOLENV_DELAY },
  { IPATCH_SF2_GEN_VOL_ENV_ATTACK, SWAMIGUI_STOCK_VOLENV_ATTACK },
  { IPATCH_SF2_GEN_VOL_ENV_HOLD, SWAMIGUI_STOCK_VOLENV_HOLD },
  { IPATCH_SF2_GEN_VOL_ENV_DECAY, SWAMIGUI_STOCK_VOLENV_DECAY },
  { IPATCH_SF2_GEN_VOL_ENV_SUSTAIN, SWAMIGUI_STOCK_VOLENV_SUSTAIN },
  { IPATCH_SF2_GEN_VOL_ENV_RELEASE, SWAMIGUI_STOCK_VOLENV_RELEASE },
  { IPATCH_SF2_GEN_ATTENUATION, GTK_STOCK_CONNECT },
  { IPATCH_SF2_GEN_NOTE_TO_VOL_ENV_HOLD, GTK_STOCK_CONNECT },
  { IPATCH_SF2_GEN_NOTE_TO_VOL_ENV_DECAY, GTK_STOCK_CONNECT },

  { SWAMIGUI_PANEL_SF2_GEN_COLUMN, NULL },

  { SWAMIGUI_PANEL_SF2_GEN_LABEL, N_("<b>Modulation Envelope</b>") },
  { IPATCH_SF2_GEN_MOD_ENV_DELAY, SWAMIGUI_STOCK_MODENV_DELAY },
  { IPATCH_SF2_GEN_MOD_ENV_ATTACK, SWAMIGUI_STOCK_MODENV_ATTACK },
  { IPATCH_SF2_GEN_MOD_ENV_HOLD, SWAMIGUI_STOCK_MODENV_HOLD },
  { IPATCH_SF2_GEN_MOD_ENV_DECAY, SWAMIGUI_STOCK_MODENV_DECAY },
  { IPATCH_SF2_GEN_MOD_ENV_SUSTAIN, SWAMIGUI_STOCK_MODENV_SUSTAIN },
  { IPATCH_SF2_GEN_MOD_ENV_RELEASE, SWAMIGUI_STOCK_MODENV_RELEASE },
  { IPATCH_SF2_GEN_MOD_ENV_TO_PITCH, GTK_STOCK_CONNECT },
  { IPATCH_SF2_GEN_MOD_ENV_TO_FILTER_CUTOFF, GTK_STOCK_CONNECT },
  { IPATCH_SF2_GEN_NOTE_TO_MOD_ENV_HOLD, GTK_STOCK_CONNECT },
  { IPATCH_SF2_GEN_NOTE_TO_MOD_ENV_DECAY, GTK_STOCK_CONNECT },

  { SWAMIGUI_PANEL_SF2_GEN_END, NULL }
};

static void swamigui_panel_sf2_gen_env_panel_iface_init (SwamiguiPanelIface *panel_iface);


G_DEFINE_TYPE_WITH_CODE (SwamiguiPanelSF2GenEnv, swamigui_panel_sf2_gen_env,
			 SWAMIGUI_TYPE_PANEL_SF2_GEN,
		    G_IMPLEMENT_INTERFACE (SWAMIGUI_TYPE_PANEL,
					   swamigui_panel_sf2_gen_env_panel_iface_init));

static void
swamigui_panel_sf2_gen_env_class_init (SwamiguiPanelSF2GenEnvClass *klass)
{
}

static void
swamigui_panel_sf2_gen_env_panel_iface_init (SwamiguiPanelIface *panel_iface)
{
  panel_iface->label = _("Envelopes");
  panel_iface->blurb = _("Controls for SoundFont envelope parameters");
  panel_iface->stockid = SWAMIGUI_STOCK_VOLENV;
}

static void
swamigui_panel_sf2_gen_env_init (SwamiguiPanelSF2GenEnv *genpanel)
{
  swamigui_panel_sf2_gen_set_controls (SWAMIGUI_PANEL_SF2_GEN (genpanel),
                                       sf2_gen_env_ctrl_info);
}

/**
 * swamigui_panel_sf2_gen_env_new:
 *
 * Create a new generator control panel object.
 *
 * Returns: new widget of type #SwamiguiPanelSF2GenEnv
 */
GtkWidget *
swamigui_panel_sf2_gen_env_new (void)
{
  return (GTK_WIDGET (gtk_type_new (swamigui_panel_sf2_gen_env_get_type ())));
}
