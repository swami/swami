/*
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
/**
 * SECTION: SwamiguiPanelSF2GenMisc
 * @short_description: SoundFont envelope generator control panel
 * @see_also: 
 * @stability: 
 */
#ifndef __SWAMIGUI_PANEL_SF2_GEN_MISC_H__
#define __SWAMIGUI_PANEL_SF2_GEN_MISC_H__

#include <gtk/gtk.h>
#include <libinstpatch/libinstpatch.h>

typedef struct _SwamiguiPanelSF2GenMisc SwamiguiPanelSF2GenMisc;
typedef struct _SwamiguiPanelSF2GenMiscClass SwamiguiPanelSF2GenMiscClass;

#include <swamigui/SwamiguiPanelSF2Gen.h>

#define SWAMIGUI_TYPE_PANEL_SF2_GEN_MISC   (swamigui_panel_sf2_gen_misc_get_type ())
#define SWAMIGUI_PANEL_SF2_GEN_MISC(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWAMIGUI_TYPE_PANEL_SF2_GEN_MISC, \
   SwamiguiPanelSF2GenMisc))
#define SWAMIGUI_PANEL_SF2_GEN_MISC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), SWAMIGUI_TYPE_PANEL_SF2_GEN_MISC, \
   SwamiguiPanelSF2GenMiscClass))
#define SWAMIGUI_IS_PANEL_SF2_GEN_MISC(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWAMIGUI_TYPE_PANEL_SF2_GEN_MISC))
#define SWAMIGUI_IS_PANEL_SF2_GEN_MISC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), SWAMIGUI_TYPE_PANEL_SF2_GEN_MISC))

struct _SwamiguiPanelSF2GenMisc
{
  SwamiguiPanelSF2Gen parent_instance;
};

struct _SwamiguiPanelSF2GenMiscClass
{
  SwamiguiPanelSF2GenClass parent_class;
};

GType swamigui_panel_sf2_gen_misc_get_type (void);
GtkWidget *swamigui_panel_sf2_gen_misc_new (void);

#endif
