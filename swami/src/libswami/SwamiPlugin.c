/*
 * SwamiPlugin.c - Swami plugin system
 *
 * Swami
 * Copyright (C) 1999-2010 Joshua "Element" Green <jgreen@users.sourceforge.net>
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
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include "config.h"

#include <libinstpatch/misc.h>	/* ipatch_gerror_message() */

#include "SwamiPlugin.h"
#include "SwamiLog.h"
#include "version.h"
#include "i18n.h"

enum
{
  PROP_0,
  PROP_MODULE,
  PROP_LOADED,
  PROP_NAME,
  PROP_FILENAME,
  PROP_VERSION,
  PROP_AUTHOR,
  PROP_COPYRIGHT,
  PROP_DESCR,
  PROP_LICENSE
};

static GList *swami_plugins = NULL; /* global list of plugins */
static int plugin_seqno = 0;	/* plugin counter (for stats) */
static GList *plugin_paths = NULL; /* plugin search paths */

static GObjectClass *parent_class = NULL;


static void swami_plugin_class_init (SwamiPluginClass *klass);
static void swami_plugin_finalize (GObject *object);
static void swami_plugin_set_property (GObject *object, guint property_id,
				       const GValue *value, GParamSpec *pspec);
static void swami_plugin_get_property (GObject *object, guint property_id,
				       GValue *value, GParamSpec *pspec);
static gboolean swami_plugin_type_module_load (GTypeModule *type_module);
static void swami_plugin_type_module_unload (GTypeModule *type_module);
static gboolean swami_plugin_load_recurse (char *file, char *name);


/* initialize plugin system */
void
_swami_plugin_initialize (void)
{
  plugin_paths = g_list_append (plugin_paths, PLUGINS_DIR);
}

GType
swami_plugin_get_type (void)
{
  static GType obj_type = 0;

  if (!obj_type)
    {
      static GTypeInfo obj_info =
	{
	  sizeof (SwamiPluginClass), NULL, NULL,
	  (GClassInitFunc) swami_plugin_class_init, NULL, NULL,
	  sizeof (SwamiPlugin), 0,
	  (GInstanceInitFunc) NULL,
	};

      obj_type = g_type_register_static (G_TYPE_TYPE_MODULE, "SwamiPlugin",
					 &obj_info, 0);
    }

  return (obj_type);
}

static void
swami_plugin_class_init (SwamiPluginClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  GTypeModuleClass *module_class = G_TYPE_MODULE_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  obj_class->finalize = swami_plugin_finalize;
  obj_class->set_property = swami_plugin_set_property;
  obj_class->get_property = swami_plugin_get_property;

  module_class->load = swami_plugin_type_module_load;
  module_class->unload = swami_plugin_type_module_unload;

  g_object_class_install_property (obj_class, PROP_MODULE,
				   g_param_spec_pointer ("module", "Module",
							 "GModule of plugin",
							 G_PARAM_READABLE));
  g_object_class_install_property (obj_class, PROP_LOADED,
			g_param_spec_boolean ("loaded", "Loaded",
					      "Loaded state of plugin",
					      FALSE,
					      G_PARAM_READABLE));

  g_object_class_install_property (obj_class, PROP_NAME,
				   g_param_spec_string ("name", "Name",
							"Name of plugin",
							NULL,
							G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_FILENAME,
			g_param_spec_string ("file-name", "File Name",
					     "Name of plugin file",
					     NULL,
					     G_PARAM_READABLE));
  g_object_class_install_property (obj_class, PROP_VERSION,
				g_param_spec_string ("version", "Version",
						     "Plugin specific version",
						     NULL,
						     G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_AUTHOR,
				   g_param_spec_string ("author", "Author",
							"Author of the plugin",
							NULL,
							G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_COPYRIGHT,
				g_param_spec_string ("copyright", "Copyright",
						     "Copyright string",
						     NULL,
						     G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_DESCR,
				g_param_spec_string ("descr", "Description",
						     "Description of plugin",
						     NULL,
						     G_PARAM_READWRITE));
  g_object_class_install_property (obj_class, PROP_LICENSE,
				   g_param_spec_string ("license", "License",
							"Plugin license type",
							NULL,
							G_PARAM_READWRITE));
}

static void
swami_plugin_finalize (GObject *object)
{
  SwamiPlugin *plugin = SWAMI_PLUGIN (object);

  g_free (plugin->filename);
  g_free (plugin->version);
  g_free (plugin->author);
  g_free (plugin->copyright);
  g_free (plugin->descr);
  g_free (plugin->license);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
swami_plugin_set_property (GObject *object, guint property_id,
			   const GValue *value, GParamSpec *pspec)
{
  SwamiPlugin *plugin = SWAMI_PLUGIN (object);

  switch (property_id)
    {
    case PROP_NAME:
      g_type_module_set_name (G_TYPE_MODULE (plugin),
			      g_value_get_string (value));
      break;
    case PROP_VERSION:
      g_free (plugin->version);
      plugin->version = g_value_dup_string (value);
      break;
    case PROP_AUTHOR:
      g_free (plugin->author);
      plugin->author = g_value_dup_string (value);
      break;
    case PROP_COPYRIGHT:
      g_free (plugin->copyright);
      plugin->copyright = g_value_dup_string (value);
      break;
    case PROP_DESCR:
      g_free (plugin->descr);
      plugin->descr = g_value_dup_string (value);
      break;
    case PROP_LICENSE:
      g_free (plugin->license);
      plugin->license = g_value_dup_string (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
swami_plugin_get_property (GObject *object, guint property_id, GValue *value,
			    GParamSpec *pspec)
{
  SwamiPlugin *plugin = SWAMI_PLUGIN (object);

  switch (property_id)
    {
    case PROP_MODULE:
      g_value_set_pointer (value, plugin->module);
      break;
    case PROP_LOADED:
      g_value_set_boolean (value, plugin->module != NULL);
      break;
    case PROP_NAME:
      g_value_set_string (value, G_TYPE_MODULE (plugin)->name);
      break;
    case PROP_FILENAME:
      g_value_set_string (value, plugin->filename);
      break;
    case PROP_VERSION:
      g_value_set_string (value, plugin->version);
      break;
    case PROP_AUTHOR:
      g_value_set_string (value, plugin->author);
      break;
    case PROP_COPYRIGHT:
      g_value_set_string (value, plugin->copyright);
      break;
    case PROP_DESCR:
      g_value_set_string (value, plugin->descr);
      break;
    case PROP_LICENSE:
      g_value_set_string (value, plugin->license);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean
swami_plugin_type_module_load (GTypeModule *type_module)
{
  SwamiPlugin *plugin = SWAMI_PLUGIN (type_module);
  SwamiPluginInfo *info;
  GError *err = NULL;

  g_return_val_if_fail (SWAMI_IS_PLUGIN (plugin), FALSE);
  g_return_val_if_fail (plugin->filename != NULL, FALSE);
  g_return_val_if_fail (plugin->module == NULL, FALSE);

  plugin->module = g_module_open (plugin->filename, G_MODULE_BIND_LAZY);
  if (!plugin->module)
    {
      g_critical (_("Failed to load plugin \"%s\": %s"), plugin->filename,
		  g_module_error ());
      return (FALSE);
    }

  if (!g_module_symbol (plugin->module, "swami_plugin_info",
			(gpointer *)&info))
    {
      g_critical (_("Symbol \"swami_plugin_info\" not found in plugin \"%s\""),
		  plugin->filename);
      goto fail;
    }

  if (strncmp (info->magic, SWAMI_PLUGIN_MAGIC, 4) != 0)
    {
      g_critical (_("Invalid Swami plugin magic number"));
      goto fail;
    }

  if (strcmp (info->swami_version, SWAMI_VERSION) != 0)
    {
      g_critical (_("Plugin compiled for Swami version %s but loading on %s"),
		  info->swami_version, SWAMI_VERSION);
      goto fail;
    }

  if (info->init && !(*info->init)(plugin, &err))
    {
      g_critical (_("Plugin init function failed: %s"),
		  ipatch_gerror_message (err));
      g_clear_error (&err);
      goto fail;
    }

  plugin->init = info->init;
  plugin->exit = info->exit;

  return (TRUE);

 fail:
  g_module_close (plugin->module);
  plugin->module = NULL;

  return (FALSE);
}

static void
swami_plugin_type_module_unload (GTypeModule *type_module)
{
  SwamiPlugin *plugin = SWAMI_PLUGIN (type_module);

  g_return_if_fail (plugin->module != NULL);

  if (plugin->exit)   /* call the plugin's exit function (if any) */
    (*plugin->exit)(plugin);

  plugin->init = NULL;
  plugin->exit = NULL;

  g_module_close (plugin->module);
  plugin->module = NULL;
}

/**
 * swami_plugin_add_path:
 * @path: The directory to add to the search path
 *
 * Add a directory to the path searched for plugins.
 */
void
swami_plugin_add_path (const char *path)
{
  plugin_paths = g_list_append (plugin_paths, g_strdup (path));
}

/**
 * swami_plugin_load_all:
 *
 * Load all plugins in the plugin search path.
 */
void
swami_plugin_load_all (void)
{
  GList *path;

  path = plugin_paths;
  if (path == NULL)
    g_warning ("Plugin search path is empty");

  while (path)
    {
      g_message (_("Loading plugins from %s"), (char *)path->data);
      swami_plugin_load_recurse (path->data, NULL);
      path = g_list_next (path);
    }

  g_message (_("Loaded %d plugins"), plugin_seqno);
}

static gboolean
swami_plugin_load_recurse (char *file, char *name)
{
  DIR *dir;
  struct dirent *dirent;
  gboolean loaded = FALSE;
  char *dirname, *temp;

  dir = opendir (file);
  if (dir)			/* is "file" a directory or file? */
    {
      while ((dirent = readdir (dir))) /* its a directory, open it */
	{
	  /* don't want to recurse in place or backwards */
	  if (strcmp (dirent->d_name, ".") && strcmp (dirent->d_name, ".."))
	    {
	      dirname = g_strjoin (G_DIR_SEPARATOR_S, file,
				   dirent->d_name, NULL);
	      loaded = swami_plugin_load_recurse (dirname, name);
	      g_free (dirname);
	      if (loaded && name) /* done searching for a specific plugin? */
		{
		  closedir (dir);
		  return TRUE;
		}
	    }
	}
      closedir (dir);
    }
  else
    { /* search for shared library extension */
      temp = g_strrstr (file, "." G_MODULE_SUFFIX);

      /* make sure file ends with shared library extension */
      if (temp && strcmp (temp, "." G_MODULE_SUFFIX) == 0)
	{
	  /* if we aren't searching for a specific plugin or this file name
	     matches the search, load the plugin */
	  if (!name || ((temp = strstr (file, name))
			&& strcmp (temp, name) == 0))
	    loaded = swami_plugin_load_absolute (file);
	}
    }

  return (loaded);
}

/**
 * swami_plugin_load:
 * @filename: File name of plugin to load
 *
 * Load the named plugin.  Name should be the plugin file name
 * (&quot;libplugin.so&quot; on Linux for example).
 *
 * Returns: Whether the plugin was loaded or not
 */
gboolean
swami_plugin_load (const char *filename)
{
  GList *path;
  char *pluginname;
  SwamiPlugin *plugin;

  g_return_val_if_fail (filename != NULL, FALSE);

  plugin = swami_plugin_find (filename);

  if (plugin && plugin->module) return (TRUE);

  path = plugin_paths;
  while (path)
    {
      pluginname = g_module_build_path (path->data, filename);
      if (swami_plugin_load_absolute (pluginname))
	{
	  g_free (pluginname);
	  return (TRUE);
	}
      g_free (pluginname);

      path = g_list_next (path);
    }

  return (FALSE);
}

/**
 * swami_plugin_load_absolute:
 * @filename: Name of plugin to load
 *
 * Load the named plugin.  Name should be the full path and file name of the
 * plugin to load.
 *
 * Returns: whether the plugin was loaded or not
 */
gboolean
swami_plugin_load_absolute (const char *filename)
{
  SwamiPlugin *plugin = NULL;
  GList *plugins = swami_plugins;

  g_return_val_if_fail (filename != NULL, FALSE);

  while (plugins)		/* see if plugin already exists */
    {
      SwamiPlugin *testplugin = (SwamiPlugin *)plugins->data;

      if (testplugin->filename && strcmp (testplugin->filename, filename) == 0)
	{
	  plugin = testplugin;
	  break;
	}
      plugins = g_list_next (plugins);
    }

  if (!plugin)
    {
      plugin = g_object_new (SWAMI_TYPE_PLUGIN, NULL);
      plugin->filename = g_strdup (filename);
      swami_plugins = g_list_prepend (swami_plugins, plugin);
      plugin_seqno++;
    }

  return (g_type_module_use (G_TYPE_MODULE (plugin)));
}

/**
 * swami_plugin_load_plugin:
 * @plugin: The plugin to load
 *
 * Load the given plugin.
 *
 * Returns: %TRUE on success (loaded or already loaded), %FALSE otherwise
 */
gboolean
swami_plugin_load_plugin (SwamiPlugin *plugin)
{
  g_return_val_if_fail (SWAMI_IS_PLUGIN (plugin), FALSE);

  if (plugin->module) return (TRUE);

  return (g_type_module_use (G_TYPE_MODULE (plugin)));
}

/**
 * swami_plugin_is_loaded:
 * @plugin: Plugin to query
 *
 * Queries if the plugin is loaded into memory
 *
 * Returns: %TRUE is loaded, %FALSE otherwise
 */
gboolean
swami_plugin_is_loaded (SwamiPlugin *plugin)
{
  g_return_val_if_fail (plugin != NULL, FALSE);

  return (plugin->module != NULL);
}

/**
 * swami_plugin_find:
 * @name: Name of plugin to find
 *
 * Search the list of registered plugins for one with the given plugin name
 * (not file name).
 *
 * Returns: Pointer to the #SwamiPlugin if found, %NULL otherwise
 */
SwamiPlugin *
swami_plugin_find (const char *name)
{
  GList *plugins = swami_plugins;

  g_return_val_if_fail (name != NULL, NULL);

  while (plugins)
    {
      SwamiPlugin *plugin = (SwamiPlugin *)plugins->data;

      if (G_TYPE_MODULE (plugin)->name
	  && strcmp (G_TYPE_MODULE (plugin)->name, name) == 0)
	return (plugin);

      plugins = g_list_next (plugins);
    }

  return (NULL);
}

/**
 * swami_plugin_get_list:
 *
 * get the currently loaded plugins
 *
 * Returns: a GList of SwamiPlugin elements which should be freed with
 *   g_list_free when finished with.
 */
GList *
swami_plugin_get_list (void)
{
  return (g_list_copy (swami_plugins));
}

/**
 * swami_plugin_save_xml:
 * @plugin: Swami plugin
 * @xmlnode: XML node to save plugin state to
 * @err: Location to store error info or %NULL to ignore
 *
 * Save a plugin's preferences state to XML.  Not all plugins implement this
 * method.  Check if the xml_save field is set for @plugin before calling, just
 * returns %TRUE if not implemented.
 *
 * Returns: %TRUE on success, %FALSE otherwise (in which case @err may be set)
 */
gboolean
swami_plugin_save_xml (SwamiPlugin *plugin, GNode *xmlnode, GError **err)
{
  g_return_val_if_fail (SWAMI_IS_PLUGIN (plugin), FALSE);
  g_return_val_if_fail (xmlnode != NULL, FALSE);
  g_return_val_if_fail (!err || !*err, FALSE);

  if (!plugin->save_xml) return (TRUE);

  return (plugin->save_xml (plugin, xmlnode, err));
}

/**
 * swami_plugin_load_xml:
 * @plugin: Swami plugin
 * @xmlnode: XML node to load plugin state from
 * @err: Location to store error info or %NULL to ignore
 *
 * Load a plugin's preferences state from XML.  Not all plugins implement this
 * method.  Check if the xml_load field is set for @plugin before calling, just
 * returns %TRUE if not implemented.
 *
 * Returns: %TRUE on success, %FALSE otherwise (in which case @err may be set)
 */
gboolean
swami_plugin_load_xml (SwamiPlugin *plugin, GNode *xmlnode, GError **err)
{
  g_return_val_if_fail (SWAMI_IS_PLUGIN (plugin), FALSE);
  g_return_val_if_fail (xmlnode != NULL, FALSE);
  g_return_val_if_fail (!err || !*err, FALSE);

  if (!plugin->load_xml) return (TRUE);

  return (plugin->load_xml (plugin, xmlnode, err));
}
