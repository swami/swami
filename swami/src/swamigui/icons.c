/*
 * icons.c - Swami stock icons
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
#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>

#include <libinstpatch/libinstpatch.h>

#include "config.h"

#include "icons.h"

/* icon mappings for IpatchItem categories */
struct
{
  int category;
  char *icon;
} category_icons[] = {
  { IPATCH_CATEGORY_NONE, GTK_STOCK_DIRECTORY },
  { IPATCH_CATEGORY_BASE, GTK_STOCK_DIRECTORY },
  { IPATCH_CATEGORY_PROGRAM, SWAMIGUI_STOCK_PRESET },
  { IPATCH_CATEGORY_INSTRUMENT, SWAMIGUI_STOCK_INST },
  { IPATCH_CATEGORY_INSTRUMENT_REF, SWAMIGUI_STOCK_INST },
  { IPATCH_CATEGORY_SAMPLE, SWAMIGUI_STOCK_SAMPLE },
  { IPATCH_CATEGORY_SAMPLE_REF, SWAMIGUI_STOCK_SAMPLE }
};

/* stores the registered "CustomLarge1" custom icon size enum */
int swamigui_icon_size_custom_large1;


void
_swamigui_stock_icons_init (void)
{
  GtkIconFactory *factory;
  GtkIconTheme *theme = gtk_icon_theme_get_default ();
  int prefix_len = strlen ("swamigui_");
  char *path;
  int i;

  /* !! keep synchronized with icons.h !! */
  static const char *items[] =
    {
      SWAMIGUI_STOCK_CONCAVE_NEG_BI,
      SWAMIGUI_STOCK_CONCAVE_NEG_UNI,
      SWAMIGUI_STOCK_CONCAVE_POS_BI,
      SWAMIGUI_STOCK_CONCAVE_POS_UNI,
      SWAMIGUI_STOCK_CONVEX_NEG_BI,
      SWAMIGUI_STOCK_CONVEX_NEG_UNI,
      SWAMIGUI_STOCK_CONVEX_POS_BI,
      SWAMIGUI_STOCK_CONVEX_POS_UNI,
      SWAMIGUI_STOCK_DLS,
      SWAMIGUI_STOCK_EFFECT_CONTROL,
      SWAMIGUI_STOCK_EFFECT_DEFAULT,
      SWAMIGUI_STOCK_EFFECT_GRAPH,
      SWAMIGUI_STOCK_EFFECT_SET,
      SWAMIGUI_STOCK_EFFECT_VIEW,
      SWAMIGUI_STOCK_GIG,
      SWAMIGUI_STOCK_GLOBAL_ZONE,
      SWAMIGUI_STOCK_INST,
      SWAMIGUI_STOCK_LINEAR_NEG_BI,
      SWAMIGUI_STOCK_LINEAR_NEG_UNI,
      SWAMIGUI_STOCK_LINEAR_POS_BI,
      SWAMIGUI_STOCK_LINEAR_POS_UNI,
      SWAMIGUI_STOCK_LOOP_NONE,
      SWAMIGUI_STOCK_LOOP_STANDARD,
      SWAMIGUI_STOCK_LOOP_RELEASE,
      SWAMIGUI_STOCK_MODENV,
      SWAMIGUI_STOCK_MODENV_ATTACK,
      SWAMIGUI_STOCK_MODENV_DECAY,
      SWAMIGUI_STOCK_MODENV_DELAY,
      SWAMIGUI_STOCK_MODENV_HOLD,
      SWAMIGUI_STOCK_MODENV_RELEASE,
      SWAMIGUI_STOCK_MODENV_SUSTAIN,
      SWAMIGUI_STOCK_MODULATOR_EDITOR,
      SWAMIGUI_STOCK_MODULATOR_JUNCT,
      SWAMIGUI_STOCK_MUTE,
      SWAMIGUI_STOCK_PIANO,
      SWAMIGUI_STOCK_PRESET,
      SWAMIGUI_STOCK_PYTHON,
      SWAMIGUI_STOCK_SAMPLE,
      SWAMIGUI_STOCK_SAMPLE_ROM,
      SWAMIGUI_STOCK_SAMPLE_VIEWER,
      SWAMIGUI_STOCK_SOUNDFONT,
      SWAMIGUI_STOCK_SPLITS,
      SWAMIGUI_STOCK_SWITCH_NEG_BI,
      SWAMIGUI_STOCK_SWITCH_NEG_UNI,
      SWAMIGUI_STOCK_SWITCH_POS_BI,
      SWAMIGUI_STOCK_SWITCH_POS_UNI,
      SWAMIGUI_STOCK_TREE,
      SWAMIGUI_STOCK_TUNING,
      SWAMIGUI_STOCK_VELOCITY,
      SWAMIGUI_STOCK_VOLENV,
      SWAMIGUI_STOCK_VOLENV_ATTACK,
      SWAMIGUI_STOCK_VOLENV_DECAY,
      SWAMIGUI_STOCK_VOLENV_DELAY,
      SWAMIGUI_STOCK_VOLENV_HOLD,
      SWAMIGUI_STOCK_VOLENV_RELEASE,
      SWAMIGUI_STOCK_VOLENV_SUSTAIN
    };

  /* ++ alloc path */
#ifdef SWAMI_DEVELOPER		/* use build tree for loading icons? */
  path = g_build_filename (BUILD_DIR, "src", "swamigui", "images", NULL);
#else
  path = g_build_filename (PKGDATA_DIR, "images", NULL);
#endif

  gtk_icon_theme_append_search_path (theme, path);
  g_free (path);	/* -- free path */

  factory = gtk_icon_factory_new ();
  gtk_icon_factory_add_default (factory);

  for (i = 0; i < (int) G_N_ELEMENTS (items); i++)
    {
      GtkIconSet *icon_set;
      GdkPixbuf *pixbuf;
      char *fn, *s;

      s = g_strconcat (&items[i][prefix_len], ".png", NULL);
#ifdef SWAMI_DEVELOPER		/* use build tree for loading icons? */
      fn = g_build_filename (BUILD_DIR, "src", "swamigui", "images", s, NULL);
#else  /* use installed shared directory */
      fn = g_build_filename (PKGDATA_DIR, "images", s, NULL);
#endif
      g_free (s);

      pixbuf = gdk_pixbuf_new_from_file (fn, NULL);
      g_free (fn);

      if (strcmp (items[i], SWAMIGUI_STOCK_MODULATOR_JUNCT) == 0)
	{
	  int width, height;

	  width = gdk_pixbuf_get_width (pixbuf);
	  height = gdk_pixbuf_get_height (pixbuf);

	  swamigui_icon_size_custom_large1
	    = gtk_icon_size_register ("CustomLarge1", width, height);
	}

      icon_set = gtk_icon_set_new_from_pixbuf (pixbuf);
      gtk_icon_factory_add (factory, items[i], icon_set);
      gtk_icon_set_unref (icon_set);

      g_object_unref (G_OBJECT (pixbuf));
    }

  g_object_unref (G_OBJECT (factory));

  /* set the default application icon name */
  gtk_window_set_default_icon_name ("swami-2");
}


/**
 * swamigui_icon_get_category_icon:
 * @category: IpatchItem type property "category" (IPATCH_CATEGORY_*)
 *
 * Get the icon used for the specified IpatchItem type category.
 *
 * Returns: Stock icon ID or %NULL if no icon for @category or invalid category.
 */
char *
swamigui_icon_get_category_icon (int category)
{
  int i;

  for (i = 0; i < G_N_ELEMENTS (category_icons); i++)
    {
      if (category_icons[i].category == category)
	return (category_icons[i].icon);
    }

  return NULL;
}
