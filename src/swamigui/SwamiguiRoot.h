/*
 * SwamiguiRoot.h - Main Swami user interface object
 *
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
#ifndef __SWAMIGUI_ROOT_H__
#define __SWAMIGUI_ROOT_H__

typedef struct _SwamiguiRoot SwamiguiRoot;
typedef struct _SwamiguiRootClass SwamiguiRootClass;

#include <gtk/gtk.h>
#include <libinstpatch/libinstpatch.h>
#include <libswami/libswami.h>
#include <swamigui/SwamiguiTreeStore.h>
#include <swamigui/SwamiguiTree.h>
#include <swamigui/SwamiguiStatusbar.h>

#define SWAMIGUI_TYPE_ROOT   (swamigui_root_get_type ())
#define SWAMIGUI_ROOT(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWAMIGUI_TYPE_ROOT, SwamiguiRoot))
#define SWAMIGUI_ROOT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), SWAMIGUI_TYPE_ROOT, SwamiguiRootClass))
#define SWAMIGUI_IS_ROOT(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWAMIGUI_TYPE_ROOT))
#define SWAMIGUI_IS_ROOT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), SWAMIGUI_TYPE_ROOT))

typedef enum
{
    SWAMIGUI_QUIT_CONFIRM_ALWAYS,	/* always pop a quit confirmation */
    SWAMIGUI_QUIT_CONFIRM_UNSAVED,	/* only if there are unsaved files */
    SWAMIGUI_QUIT_CONFIRM_NEVER	/* spontaneous combust */
} SwamiguiQuitConfirm;

/* Swami User Interface Object */
struct _SwamiguiRoot
{
    SwamiRoot parent_instance;	/* derived from SwamiRoot */

    SwamiguiTreeStore *patch_store; /* patch tree store */
    SwamiguiTreeStore *config_store; /* config tree store */
    IpatchList *tree_stores;	 /* list of tree stores (including above) */

    IpatchList *selection;	/* most recent item selection (trees, etc) */

    GtkWidget *main_window;	/* Main toplevel window */
    GtkWidget *tree;		/* Tree widget */
    GtkWidget *splits;		/* Note/velocity splits widget */
    gboolean splits_changed;      /* Set to TRUE if splits-item changed and needs updating */
    GtkWidget *panel_selector;	/* Panel selector widget */
    SwamiguiStatusbar *statusbar;	/* Main statusbar */

    SwamiWavetbl *wavetbl;	/* Wavetable object */
    gboolean solo_item_enabled;   /* TRUE if solo item enabled, FALSE otherwise */
    GObject *solo_item;           /* Solo item child of active-item or NULL */
    char *solo_item_icon;         /* Stores original icon of current solo item or NULL */

    SwamiControlQueue *ctrl_queue; /* control update queue */
    guint update_timeout_id;	/* GSource ID for GUI update timeout */
    int update_interval;		/* GUI update interval in milliseconds */

    SwamiControlFunc *ctrl_prop; /* patch item property change ctrl listener */
    SwamiControlFunc *ctrl_add;	/* patch item add control listener */
    SwamiControlFunc *ctrl_remove; /* patch item remove control listener */
    GSList *ctrl_list; /* properties controls list created at initialization*/

    SwamiguiQuitConfirm quit_confirm; /* quit confirm enum */
    gboolean splash_enable;	/* show splash on startup? */
    guint splash_delay;		/* splash delay in milliseconds (0 to wait for button press) */
    gboolean tips_enable;		/* display next tip on startup? */
    int tips_position;		/* Swami tips position */
    guint *piano_lower_keys;	/* zero terminated array of keys (or NULL) */
    guint *piano_upper_keys;	/* zero terminated array of keys (or NULL) */
    GType default_patch_type;	/* default patch type (for File->New) */

    GNode *loaded_xml_config;     /* Last loaded XML config (usually only on startup) or NULL */
    GList *panel_cache;           /* Cache of SwamiguiPanel objects */
    gboolean middle_emul_enable;  /* Middle mouse button emulation enable */
    gboolean middle_emul_mod;     /* Middle mouse button emulation key modifier */
};

struct _SwamiguiRootClass
{
    SwamiRootClass parent_class;

    /* signals */
    void (*quit)(SwamiguiRoot *root);
};

// Name of instrument files group for GtkRecentManager items
#define SWAMIGUI_ROOT_INSTRUMENT_FILES_GROUP  "Instrument Files"

/* global instances of SwamiguiRoot */
extern SwamiguiRoot *swamigui_root;
extern SwamiRoot *swami_root;	/* just for convenience */

/* Getter returning swamigui_root. */
/* Useful when libswamigui is used as a shared library */
SwamiguiRoot *swamigui_get_swamigui_root(void);

void swamigui_init(int *argc, char **argv[]);
GType swamigui_root_get_type(void);
SwamiguiRoot *swamigui_root_new(void);
void swamigui_root_activate(SwamiguiRoot *root);
void swamigui_root_quit(SwamiguiRoot *root);
SwamiguiRoot *swamigui_get_root(gpointer gobject);
gboolean swamigui_root_save_prefs(SwamiguiRoot *root);
gboolean swamigui_root_load_prefs(SwamiguiRoot *root);
gboolean swamigui_root_patch_load(SwamiRoot *root, const char *filename,
                                  IpatchItem **item, GtkWindow *parent);
gboolean swamigui_root_is_middle_click(SwamiguiRoot *root, GdkEventButton *event);

#endif

