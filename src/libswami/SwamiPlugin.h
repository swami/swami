/*
 * SwamiPlugin.h - Header file for Swami plugin system
 *
 * Swami
 * Copyright (C) 1999-2014 Element Green <element@elementsofsound.org>
 *
 * Inspired by gstplugin from GStreamer (although re-coded)
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
#ifndef __SWAMI_PLUGIN_H__
#define __SWAMI_PLUGIN_H__

#include <glib.h>
#include <glib-object.h>
#include <gmodule.h>

#include <libswami/version.h>

typedef struct _SwamiPlugin SwamiPlugin;
typedef struct _SwamiPluginClass SwamiPluginClass;
typedef struct _SwamiPluginInfo SwamiPluginInfo;

#define SWAMI_TYPE_PLUGIN   (swami_plugin_get_type ())
#define SWAMI_PLUGIN(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWAMI_TYPE_PLUGIN, SwamiPlugin))
#define SWAMI_PLUGIN_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), SWAMI_TYPE_PLUGIN, SwamiPluginClass))
#define SWAMI_IS_PLUGIN(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWAMI_TYPE_PLUGIN))
#define SWAMI_IS_PLUGIN_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), SWAMI_TYPE_PLUGIN))

/**
 * SwamiPluginInitFunc:
 * @plugin: Plugin object being loaded
 * @err: Location to store error information
 *
 * A function type called after a plugin has been loaded.
 *
 * Returns: Should return %TRUE on success, %FALSE otherwise (@err should be
 * set on failure).
 */
typedef gboolean(*SwamiPluginInitFunc)(SwamiPlugin *plugin, GError **err);

/**
 * SwamiPluginExitFunc:
 * @plugin: Plugin object being unloaded
 *
 * A function type called before a plugin is unloaded.
 */
typedef void (*SwamiPluginExitFunc)(SwamiPlugin *plugin);

/**
 * SwamiPluginSaveXmlFunc:
 * @plugin: Plugin object to save preference state of
 * @xmlnode: #IpatchXmlNode tree to save preferences to
 * @err: Location to store error info to
 *
 * An optional function type which is called to save a plugin's preference state to
 * an XML tree.  The passed in @xmlnode is an XML node created for this plugin.
 *
 * Returns: Should return %TRUE on success, %FALSE otherwise (in which case @err
 *   should be set).
 */
typedef gboolean(*SwamiPluginSaveXmlFunc)(SwamiPlugin *plugin, GNode *xmlnode,
        GError **err);

/**
 * SwamiPluginLoadXmlFunc:
 * @plugin: Plugin object to load preference state to
 * @xmlnode: #IpatchXmlNode tree to load preferences from
 * @err: Location to store error info to
 *
 * An optional function type which is called to load a plugin's preference state from
 * an XML tree.  The passed in @xmlnode is an XML node for this plugin.
 *
 * Returns: Should return %TRUE on success, %FALSE otherwise (in which case @err
 *   should be set).
 */
typedef gboolean(*SwamiPluginLoadXmlFunc)(SwamiPlugin *plugin, GNode *xmlnode,
        GError **err);

/* Swami plugin object (each loaded plugin gets one of these) */
struct _SwamiPlugin
{
    GTypeModule parent_instance;	/* derived from GTypeModule */

    /*< private >*/
    GModule *module;    /* module of the plugin or NULL if not loaded */
    SwamiPluginInitFunc init;	/* function stored from SwamiPluginInfo */
    SwamiPluginExitFunc exit;	/* function stored from SwamiPluginInfo */
    SwamiPluginSaveXmlFunc save_xml;	/* Optional XML save function assigned in init */
    SwamiPluginLoadXmlFunc load_xml;	/* Optional XML load function assigned in init */
    char *filename;		/* filename it came from */

    char *version;		/* plugin specific version string */
    char *author;			/* author of this plugin */
    char *copyright;		/* copyright string */
    char *descr;			/* description of plugin */
    char *license;		/* license this plugin is distributed under */
};

/* Swami plugin class */
struct _SwamiPluginClass
{
    GTypeModuleClass parent_class;
};

/* magic string to check sanity of plugins */
//#define SWAMI_PLUGIN_MAGIC GUINT_FROM_BE(0x53574D49)
#define SWAMI_PLUGIN_MAGIC "SWMI"

struct _SwamiPluginInfo
{
    char magic[4];		/* magic string to ensure sanity */
    char *swami_version;		/* version of Swami plugin compiled for */
    SwamiPluginInitFunc init;	/* called to initialize plugin */
    SwamiPluginExitFunc exit;	/* called before plugin is unloaded */
};

/* a convenience macro to define plugin info */
#define SWAMI_PLUGIN_INFO(init, exit)	\
SwamiPluginInfo swami_plugin_info =	\
{					\
  SWAMI_PLUGIN_MAGIC,			\
  SWAMI_VERSION,			\
  init,					\
  exit					\
};

GType swami_plugin_get_type(void);

void swami_plugin_add_path(const char *path);
void swami_plugin_load_all(void);
gboolean swami_plugin_load(const char *filename);
gboolean swami_plugin_load_absolute(const char *filename);
gboolean swami_plugin_load_plugin(SwamiPlugin *plugin);
gboolean swami_plugin_is_loaded(SwamiPlugin *plugin);
SwamiPlugin *swami_plugin_find(const char *name);
GList *swami_plugin_get_list(void);
gboolean swami_plugin_save_xml(SwamiPlugin *plugin, GNode *xmlnode, GError **err);
gboolean swami_plugin_load_xml(SwamiPlugin *plugin, GNode *xmlnode, GError **err);

#endif /* __SWAMI_PLUGIN_H__ */
