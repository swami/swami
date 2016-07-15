/*
 * libInstPatch
 * Copyright (C) 1999-2014 Element Green <element@elementsofsound.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; version 2.1
 * of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA or on the web at http://www.gnu.org.
 */
#ifndef __IPATCH_CONVERTER_H__
#define __IPATCH_CONVERTER_H__

#include <stdarg.h>
#include <glib.h>
#include <glib-object.h>
#include <libinstpatch/IpatchList.h>

/* forward type declarations */

typedef struct _IpatchConverter IpatchConverter;
typedef struct _IpatchConverterClass IpatchConverterClass;
typedef struct _IpatchConverterInfo IpatchConverterInfo;

#define IPATCH_TYPE_CONVERTER   (ipatch_converter_get_type ())
#define IPATCH_CONVERTER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), IPATCH_TYPE_CONVERTER, IpatchConverter))
#define IPATCH_CONVERTER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), IPATCH_TYPE_CONVERTER, \
   IpatchConverterClass))
#define IPATCH_IS_CONVERTER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPATCH_TYPE_CONVERTER))
#define IPATCH_IS_CONVERTER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), IPATCH_TYPE_CONVERTER))
#define IPATCH_CONVERTER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), IPATCH_TYPE_CONVERTER, \
   IpatchConverterClass))

/**
 * IpatchConverterLinkLookupFunc:
 * @converter: Converter instance
 * @item: Converted item which has the link property that will be assigned to
 * @link: Original object being linked (before conversion)
 * @newtype: New type that link needs to be converted to
 * @user_data: User defined data supplied to the converter instance
 *
 * This function type allows for object link interception by the user of
 * an #IpatchConverter instance.  It is called by conversion processes which
 * create objects linking other external objects which need to be converted
 * also.  If this function returns %NULL then the @link will be converted
 * by the converter process and the user notified with the
 * #IpatchConverterLinkNotifyFunc.  An example usage of this feature is
 * the #IpatchPaste system, which does object conversions and substitutes
 * already converted objects (a conversion pool).
 *
 * Returns: Existing converted item satisfying @link and @newtype, or %NULL
 * otherwise.
 */
typedef GObject * (*IpatchConverterLinkLookupFunc)
  (IpatchConverter *converter, GObject *item, GObject *link, GType newtype,
   gpointer user_data);

/**
 * IpatchConverterLinkNotifyFunc:
 * @converter: Converter instance
 * @orig: Original link item which was converted
 * @conv: New converted link item
 * @newtype: New type that link was converted to (same as passed to
 *   #IpatchConverterLinkLookupFunc).
 * @user_data: User defined data supplied to the converter instance
 *
 * This function type allows for object link interception by the user of
 * an #IpatchConverter instance.  It is called by conversion processes which
 * create objects linking other external objects which need to be converted
 * also.  For each link object which needs to be converted
 * #IpatchConverterLinkLookupFunc is called first, if it returns %NULL then
 * this function will be called with the newly converted link object.
 */
typedef void (*IpatchConverterLinkNotifyFunc)
  (IpatchConverter *converter, GObject *orig, GObject *conv, GType newtype,
   gpointer user_data);

/* conversion instance */
struct _IpatchConverter
{
  GObject parent_instance;	/* derived from GObject */

  int flags;			/* IpatchConverterFlags */

  GList *inputs;	       /* list of input GObjects to convert */
  GList *outputs;	   /* list of new converted output GObjects */

  /* callbacks for object link interception */
  IpatchConverterLinkLookupFunc *link_lookup;
  IpatchConverterLinkNotifyFunc *link_notify;
  GDestroyNotify notify_func;   // Callback for when the above callbacks are removed/replaced
  gpointer user_data;           // User data passed to notify_func (not the other callbacks)

  float progress;		/* 0.0 - 1.0 progress property */

  /* conversion ratings (0.0 - 1.0 = worst - best). For container objects
     ratings can be done individually on the children, then min_rate/max_rate
     will be useful */
  float min_rate;		/* minimum rating amongst all items */
  float max_rate;		/* maximum rating amongst all items */
  float avg_rate;		/* average rating for all items */
  float sum_rate;		/* sum of all ratings (to calculate avg) */
  int item_count;	     /* count of children items being rated */

  gboolean rate_items; /* set to TRUE to log a rating for each child item */

  /* conversion log */
  GList *log; /* LogEntry list (defined in IpatchConverter.c, prepended) */
};

/* conversion class */
struct _IpatchConverterClass
{
  GObjectClass parent_class;

  /* methods */
  gboolean (*verify)(IpatchConverter *converter, char **failmsg);
  void (*init)(IpatchConverter *converter);
  gboolean (*convert)(IpatchConverter *converter, GError **err);
  char * (*notes)(IpatchConverter *converter);
};

/* type for log entries */
typedef enum
{
  IPATCH_CONVERTER_LOG_RATING,	/* log a rating update */
  IPATCH_CONVERTER_LOG_INFO,	/* informational only */
  IPATCH_CONVERTER_LOG_WARN,	/* warning */
  IPATCH_CONVERTER_LOG_CRITICAL, /* critical (but non fatal) message */
  IPATCH_CONVERTER_LOG_FATAL	/* fatal error */
} IpatchConverterLogType;

/* mask for type field (IpatchConverterLogType) */
#define IPATCH_CONVERTER_LOG_TYPE_MASK   0x0F

/* flag for IpatchConverterLog->type to indicate allocated message string */
#define IPATCH_CONVERTER_LOG_MSG_ALLOC 0x80

#define IPATCH_CONVERTER_INPUT(converter) \
  (converter->inputs ? G_OBJECT (converter->inputs->data) : (GObject *)NULL)

#define IPATCH_CONVERTER_OUTPUT(converter) \
  (converter->outputs ? G_OBJECT (converter->outputs->data) : (GObject *)NULL)

/* enum used for src_count and dest_count fields in class */
typedef enum
{
  IPATCH_CONVERTER_COUNT_ONE_OR_MORE  = -1,	/* 1 or more objects */
  IPATCH_CONVERTER_COUNT_ZERO_OR_MORE = -2	/* 0 or more objects */
} IpatchConverterCount;

/**
 * IpatchConverterFlags:
 * @IPATCH_CONVERTER_SRC_DERIVED: Match source derived types also (type descendants of src_type)
 * @IPATCH_CONVERTER_DEST_DERIVED: Match destination derived types also (type descendants of dest_type)
 *
 * Flags for ipatch_register_converter_map()
 *
 * Since: 1.1.0
 */
typedef enum
{
  IPATCH_CONVERTER_SRC_DERIVED     = 1 << 8,
  IPATCH_CONVERTER_DEST_DERIVED    = 1 << 9,
} IpatchConverterFlags;

/* priority levels for converter mappings */
typedef enum
{
  /* 0 value is an alias for IPATCH_CONVERTER_PRIORITY_DEFAULT */

  IPATCH_CONVERTER_PRIORITY_LOWEST  = 1,
  IPATCH_CONVERTER_PRIORITY_LOW     = 25,
  IPATCH_CONVERTER_PRIORITY_DEFAULT = 50,
  IPATCH_CONVERTER_PRIORITY_HIGH    = 75,
  IPATCH_CONVERTER_PRIORITY_HIGHEST = 100
} IpatchConverterPriority;

/**
 * IpatchConverterInfo:
 * @conv_type: Conversion handler type
 * @src_type: Source type of conversion handler
 * @src_match: Furthest source parent type to match (0 = exact match)
 * @dest_type: Destination type of conversion handler
 * @dest_match: Furthest dest parent type to match (0 = exact match)
 * @flags: #IpatchConverterFlags | priority (#IpatchConverterPriority value or other integer value)
 * @src_count: Required source item count or IpatchConverterCount
 * @dest_count: Required destination item count or IpatchConverterCount
 *
 * Registered object converter information.
 */
struct _IpatchConverterInfo
{
  GType conv_type;
  GType src_type;
  GType src_match;
  GType dest_type;
  GType dest_match;
  guint8 flags;
  guint8 priority;
  gint8 src_count;
  gint8 dest_count;
};

gboolean ipatch_convert_objects (GObject *input, GObject *output, GError **err);
GObject *ipatch_convert_object_to_type (GObject *object, GType type,
					GError **err);
IpatchList *ipatch_convert_object_to_type_multi (GObject *object, GType type,
                                                 GError **err);
GList *ipatch_convert_object_to_type_multi_list (GObject *object, GType type, GError **err);
IpatchList *ipatch_convert_object_to_type_multi_set (GObject *object, GType type,
                                                     GError **err,
                                                     const char *first_property_name, ...);
GList *ipatch_convert_object_to_type_multi_set_vlist (GObject *object, GType type, GError **err,
                                                      const char *first_property_name, va_list args);

IpatchConverter *ipatch_create_converter (GType src_type, GType dest_type);
IpatchConverter *ipatch_create_converter_for_objects (GObject *input, GObject *output, GError **err);
IpatchConverter *ipatch_create_converter_for_object_to_type (GObject *object, GType dest_type, GError **err);

void ipatch_register_converter_map (GType conv_type, guint8 flags, guint8 priority,
                                    GType src_type, GType src_match, gint8 src_count,
                                    GType dest_type, GType dest_match, gint8 dest_count);
GType ipatch_find_converter (GType src_type, GType dest_type);
GType *ipatch_find_converters (GType src_type, GType dest_type, guint flags);
const IpatchConverterInfo *ipatch_lookup_converter_info (GType conv_type, GType src_type, GType dest_type);
const IpatchConverterInfo *ipatch_get_converter_info (GType conv_type);

GType ipatch_converter_get_type (void);
void ipatch_converter_add_input (IpatchConverter *converter, GObject *object);
void ipatch_converter_add_output (IpatchConverter *converter, GObject *object);
void ipatch_converter_add_inputs (IpatchConverter *converter, GList *objects);
void ipatch_converter_add_outputs (IpatchConverter *converter, GList *objects);
GObject *ipatch_converter_get_input (IpatchConverter *converter);
GObject *ipatch_converter_get_output (IpatchConverter *converter);
IpatchList *ipatch_converter_get_inputs (IpatchConverter *converter);
GList *ipatch_converter_get_inputs_list (IpatchConverter *converter);
IpatchList *ipatch_converter_get_outputs (IpatchConverter *converter);
GList *ipatch_converter_get_outputs_list (IpatchConverter *converter);

gboolean ipatch_converter_verify (IpatchConverter *converter, char **failmsg);
void ipatch_converter_init (IpatchConverter *converter);
gboolean ipatch_converter_convert (IpatchConverter *converter, GError **err);
void ipatch_converter_reset (IpatchConverter *converter);

char *ipatch_converter_get_notes (IpatchConverter *converter);

void ipatch_converter_log (IpatchConverter *converter, GObject *item,
			   int type, char *msg);
void ipatch_converter_log_printf (IpatchConverter *converter, GObject *item,
				  int type, const char *fmt, ...);
gboolean ipatch_converter_log_next (IpatchConverter *converter, gpointer *pos,
				    GObject **item, int *type, char **msg);

void ipatch_converter_set_link_funcs (IpatchConverter *converter,
				IpatchConverterLinkLookupFunc *link_lookup,
				IpatchConverterLinkNotifyFunc *link_notify);
void
ipatch_converter_set_link_funcs_full (IpatchConverter *converter,
                                      IpatchConverterLinkLookupFunc *link_lookup,
                                      IpatchConverterLinkNotifyFunc *link_notify,
                                      GDestroyNotify notify_func, gpointer user_data);
#endif
