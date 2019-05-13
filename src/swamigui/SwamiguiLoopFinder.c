/*
 * SwamiguiLoopFinder.c - Sample loop finder widget
 *
 * Swami
 * Copyright (C) 1999-2014 Element Green <element@elementsofsound.org>
 *
 * Thanks to Luis Garrido for the loop finder algorithm code and his
 * interest in creating this feature for Swami.
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

#include <libinstpatch/libinstpatch.h>
#include <libswami/SwamiLog.h>

#include "SwamiguiLoopFinder.h"
#include "SwamiguiControl.h"
#include "SwamiguiRoot.h"
#include "util.h"
#include "i18n.h"

enum
{
  PROP_0,
  PROP_FINDER
};

/* columns for result list store */
enum
{
  SIZE_COLUMN,
  START_COLUMN,
  END_COLUMN,
  QUALITY_COLUMN
};

/* GUI worker thread monitor callback interval in milliseconds */
#define PROGRESS_UPDATE_INTERVAL  100

static void swamigui_loop_finder_set_property (GObject *object,
					       guint property_id,
					       const GValue *value,
					       GParamSpec *pspec);
static void swamigui_loop_finder_get_property (GObject *object,
					       guint property_id,
					       GValue *value,
					       GParamSpec *pspec);
static void swamigui_loop_finder_init (SwamiguiLoopFinder *editor);
static void swamigui_loop_finder_finalize (GObject *object);
static void swamigui_loop_finder_destroy (GtkObject *object);
static void
swamigui_loop_finder_cb_selection_changed (GtkTreeSelection *selection,
					   gpointer user_data);
static void
swamigui_loop_finder_cb_revert (GtkButton *button, gpointer user_data);
static void swamigui_loop_finder_cb_find (GtkButton *button, gpointer user_data);
static void swamigui_loop_finder_update_find_button (SwamiguiLoopFinder *finder,
						     gboolean find);
static gboolean swamigui_loop_finder_thread_monitor (gpointer user_data);
static gpointer find_loop_thread (gpointer data);

G_DEFINE_TYPE (SwamiguiLoopFinder, swamigui_loop_finder, GTK_TYPE_VBOX);


static void
swamigui_loop_finder_class_init (SwamiguiLoopFinderClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  GtkObjectClass *gtkobj_class = GTK_OBJECT_CLASS (klass);

  obj_class->set_property = swamigui_loop_finder_set_property;
  obj_class->get_property = swamigui_loop_finder_get_property;
  obj_class->finalize = swamigui_loop_finder_finalize;

  gtkobj_class->destroy = swamigui_loop_finder_destroy;

  g_object_class_install_property (obj_class, PROP_FINDER,
		g_param_spec_object ("finder", _("Finder"),
				     _("Loop finder object"),
				     SWAMI_TYPE_LOOP_FINDER, G_PARAM_READABLE));
}

static void
swamigui_loop_finder_set_property (GObject *object, guint property_id,
				   const GValue *value, GParamSpec *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
swamigui_loop_finder_get_property (GObject *object, guint property_id,
				   GValue *value, GParamSpec *pspec)
{
  SwamiguiLoopFinder *finder = SWAMIGUI_LOOP_FINDER (object);

  switch (property_id)
    {
    case PROP_FINDER:
      g_value_set_object (value, finder->loop_finder);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
swamigui_loop_finder_init (SwamiguiLoopFinder *finder)
{
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkTreeSelection *selection;
  GtkWidget *treeview;
  GtkWidget *widg;

  /* create the loop finder object (!! takes over ref) */
  finder->loop_finder = swami_loop_finder_new ();

  /* create result list store */
  finder->store = gtk_list_store_new (4, G_TYPE_INT, G_TYPE_INT, G_TYPE_INT,
				      G_TYPE_FLOAT);
  /* create Glade GTK loop finder interface */
  finder->glade_widg = swamigui_util_glade_create ("LoopFinder");
  gtk_container_add (GTK_CONTAINER (finder), finder->glade_widg);

  /* fetch the tree view widget (result list) */
  treeview = swamigui_util_glade_lookup (finder->glade_widg, "ListMatches");

  /* Disable tree view search since it breaks piano key playback */
  g_object_set (treeview, "enable-search", FALSE, NULL);

  /* add size column to tree view widget */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Loop size"), renderer,
						     "text", SIZE_COLUMN,
						     NULL);
  gtk_tree_view_column_set_sort_column_id (column, SIZE_COLUMN);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

  /* add start column to tree view widget */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Loop start"), renderer,
						     "text", START_COLUMN,
						     NULL);
  gtk_tree_view_column_set_sort_column_id (column, START_COLUMN);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

  /* add end column to tree view widget */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Loop end"), renderer,
						     "text", END_COLUMN,
						     NULL);
  gtk_tree_view_column_set_sort_column_id (column, END_COLUMN);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

  /* add quality column to tree view widget */
  renderer = gtk_cell_renderer_progress_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Rating"), renderer,
						     "value", QUALITY_COLUMN,
						     NULL);
  gtk_tree_view_column_set_sort_column_id (column, QUALITY_COLUMN);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

  /* assign the tree store */
  gtk_tree_view_set_model (GTK_TREE_VIEW (treeview),
			   GTK_TREE_MODEL (finder->store));

  /* connect to list view selection changed signal */
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
  g_signal_connect (selection, "changed",
		    G_CALLBACK (swamigui_loop_finder_cb_selection_changed),
		    finder);

  /* connect analysis window spin button to finder's property */
  widg = swamigui_util_glade_lookup (finder->glade_widg, "SpinAnalysisWindow");
  swamigui_control_prop_connect_widget (G_OBJECT (finder->loop_finder),
					"analysis-window", G_OBJECT (widg));

  /* connect min loop spin button to finder's property */
  widg = swamigui_util_glade_lookup (finder->glade_widg, "SpinMinLoop");
  swamigui_control_prop_connect_widget (G_OBJECT (finder->loop_finder),
					"min-loop-size", G_OBJECT (widg));

  /* connect max results spin button to finder's property */
  widg = swamigui_util_glade_lookup (finder->glade_widg, "SpinMaxResults");
  swamigui_control_prop_connect_widget (G_OBJECT (finder->loop_finder),
					"max-results", G_OBJECT (widg));

  /* connect group pos diff spin button to finder's property */
  widg = swamigui_util_glade_lookup (finder->glade_widg, "SpinGroupPosDiff");
  swamigui_control_prop_connect_widget (G_OBJECT (finder->loop_finder),
					"group-pos-diff", G_OBJECT (widg));

  /* connect group size diff spin button to finder's property */
  widg = swamigui_util_glade_lookup (finder->glade_widg, "SpinGroupSizeDiff");
  swamigui_control_prop_connect_widget (G_OBJECT (finder->loop_finder),
					"group-size-diff", G_OBJECT (widg));

  widg = swamigui_util_glade_lookup (finder->glade_widg, "BtnRevert");
  g_signal_connect (widg, "clicked",
		    G_CALLBACK (swamigui_loop_finder_cb_revert), finder);

  /* find button has nothing packed in it, initialize it */
  swamigui_loop_finder_update_find_button (finder, TRUE);

  widg = swamigui_util_glade_lookup (finder->glade_widg, "BtnFind");
  g_signal_connect (widg, "clicked",
		    G_CALLBACK (swamigui_loop_finder_cb_find), finder);
}

static void
swamigui_loop_finder_finalize (GObject *object)
{
  SwamiguiLoopFinder *finder = SWAMIGUI_LOOP_FINDER (object);

  g_object_unref (finder->loop_finder);

  if (G_OBJECT_CLASS (swamigui_loop_finder_parent_class)->finalize)
    G_OBJECT_CLASS (swamigui_loop_finder_parent_class)->finalize (object);
}

/* loop finder widget destroy */
static void
swamigui_loop_finder_destroy (GtkObject *object)
{
  SwamiguiLoopFinder *finder = SWAMIGUI_LOOP_FINDER (object);

  /* signal loop finder object to stop (does nothing if not active) */
  g_object_set (finder->loop_finder, "cancel", TRUE, NULL);
}

static void
swamigui_loop_finder_cb_selection_changed (GtkTreeSelection *selection,
					   gpointer user_data)
{
  SwamiguiLoopFinder *finder = SWAMIGUI_LOOP_FINDER (user_data);
  IpatchSample *sample;
  GtkTreeModel *model;
  GtkTreeIter iter;
  guint loop_start, loop_end;

  g_object_get (finder->loop_finder, "sample", &sample, NULL);	/* ++ ref */
  if (!sample) return;

  if (!gtk_tree_selection_get_selected (selection, &model, &iter))
  {
    g_object_unref (sample);	/* -- unref */
    return;
  }

  gtk_tree_model_get (model, &iter,
		      START_COLUMN, &loop_start,
		      END_COLUMN, &loop_end,
		      -1);

  g_object_set (sample,
		"loop-start", loop_start,
		"loop-end", loop_end,
		NULL);

  g_object_unref (sample);	/* -- unref */
}

/* button to revert to original loop settings */
static void
swamigui_loop_finder_cb_revert (GtkButton *button, gpointer user_data)
{
  SwamiguiLoopFinder *finder = SWAMIGUI_LOOP_FINDER (user_data);
  IpatchSample *sample;

  g_object_get (finder->loop_finder, "sample", &sample, NULL);	/* ++ ref */
  if (!sample) return;

  /* restore original loop */
  g_object_set (sample,
		"loop-start", finder->orig_loop_start,
		"loop-end", finder->orig_loop_end,
		NULL);

  g_object_unref (sample);	/* -- unref */
}

static void
swamigui_loop_finder_cb_find (GtkButton *button, gpointer user_data)
{
  SwamiguiLoopFinder *finder = SWAMIGUI_LOOP_FINDER (user_data);
  GtkWidget *main_window;
  GtkWidget *dialog;
  IpatchSample *sample;
  GtkWidget *label;
  GError *err = NULL;
  gboolean active;

  g_object_get (finder->loop_finder,
		"sample", &sample,	/* ++ ref */
		"active", &active,
		NULL);
  if (!sample) return;

  /* store original loop start/end before any changes (for revert button) */
  g_object_get (sample, "loop-start", &finder->orig_loop_start,
		"loop-end", &finder->orig_loop_end, NULL);

  g_object_unref (sample);	/* -- unref */

  if (active)	/* if already active, tell it to stop */
  {
    g_object_set (finder->loop_finder, "cancel", TRUE, NULL);
    return;
  }

  /* clear time ellapsed label */
  label = swamigui_util_glade_lookup (finder->glade_widg, "LabelTime");
  gtk_label_set_text (GTK_LABEL (label), "");

#if 0
  {     /* For debugging */
    SwamiLoopFinder *f = finder->loop_finder;

    printf ("analysis=%d minloop=%d swinpos=%d swinsize=%d ewinpos=%d ewinsize=%d\n",
	    f->analysis_window, f->min_loop_size, f->start_window_pos, f->start_window_size,
	    f->end_window_pos, f->end_window_size);
  }
#endif

  /* verify loop finder parameters and nudge them if needed */
  if (!swami_loop_finder_verify_params (finder->loop_finder, TRUE, &err))
  {
    g_object_get (swamigui_root, "main-window", &main_window, NULL);
    dialog = gtk_message_dialog_new (GTK_WINDOW (main_window),
				     GTK_DIALOG_DESTROY_WITH_PARENT,
				     GTK_MESSAGE_ERROR,
				     GTK_BUTTONS_CLOSE,
				     _("Loop find failed: %s"),
				     ipatch_gerror_message (err));
    if (err) g_error_free (err);

    /* Destroy the dialog when the user responds to it */
    g_signal_connect_swapped (dialog, "response",
			      G_CALLBACK (gtk_widget_destroy),
			      dialog);
    return;
  }

#if 0
  {     /* For debugging */
    SwamiLoopFinder *f = finder->loop_finder;

    printf ("analysis=%d minloop=%d swinpos=%d swinsize=%d ewinpos=%d ewinsize=%d\n",
	    f->analysis_window, f->min_loop_size, f->start_window_pos, f->start_window_size,
	    f->end_window_pos, f->end_window_size);
  }
#endif

  /* create a thread to do the work */
  if (!g_thread_create (find_loop_thread, finder, FALSE, &err))
    {
      g_critical (_("Failed to start loop finder thread: %s"),
		  ipatch_gerror_message (err));
      if (err) g_error_free (err);
      return;
    }  

  /* update the find button to "Stop" state */
  swamigui_loop_finder_update_find_button (finder, FALSE);

  /* ++ ref finder for worker thread (unref'd in thread monitor timeout) */
  g_object_ref (finder);

  g_timeout_add (PROGRESS_UPDATE_INTERVAL, swamigui_loop_finder_thread_monitor,
		 finder);
}

/* modifies the find button to reflect the current state (Execute or Stop) */
static void
swamigui_loop_finder_update_find_button (SwamiguiLoopFinder *finder,
					 gboolean find)
{
  GtkWidget *btn;
  GtkWidget *hbox;
  GtkWidget *image;
  GtkWidget *label;
  GList *children, *p;

  btn = swamigui_util_glade_lookup (finder->glade_widg, "BtnFind");

  /* remove current widgets in button */
  children = gtk_container_get_children (GTK_CONTAINER (btn));
  for (p = children; p; p = p->next)
    gtk_container_remove (GTK_CONTAINER (btn), GTK_WIDGET (p->data));
  g_list_free (children);

  hbox = gtk_hbox_new (FALSE, 2);
  gtk_container_add (GTK_CONTAINER (btn), hbox);

  image = gtk_image_new_from_stock (find ? GTK_STOCK_EXECUTE : GTK_STOCK_STOP,
				    GTK_ICON_SIZE_BUTTON);
  label = gtk_label_new (find ? _("Find Loops") : _("Stop"));

  gtk_box_pack_start (GTK_BOX (hbox), image, 0, 0, 0);
  gtk_box_pack_start (GTK_BOX (hbox), label, 0, 0, 0);

  gtk_widget_show_all (hbox);
}

/* monitors worker thread activity and updates GUI */
static gboolean
swamigui_loop_finder_thread_monitor (gpointer user_data)
{
  SwamiguiLoopFinder *finder = SWAMIGUI_LOOP_FINDER (user_data);
  SwamiLoopResults *results;
  SwamiLoopMatch *matches;
  GtkTreeIter iter;
  GtkWidget *label;
  GtkWidget *prog_widg;
  float cur_progress;
  float maxq, diffq, quality;
  gboolean active;
  guint match_count;
  guint exectime;
  char *s;
  guint i;

  /* read progress float */
  g_object_get (finder->loop_finder,
		"progress", &cur_progress,
		"active", &active,
		NULL);

  /* progress changed from previous value? */
  if (cur_progress != finder->prev_progress)
  {
    prog_widg = swamigui_util_glade_lookup (finder->glade_widg, "Progress");
    gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (prog_widg), cur_progress);
    finder->prev_progress = cur_progress;
  }

  /* stop the monitor timeout callback if thread is done */
  if (!active)
  {
    /* update the time ellapsed label */
    g_object_get (finder->loop_finder, "exec-time", &exectime, NULL);
    s = g_strdup_printf (_("%0.2f secs"), exectime / 1000.0);
    label = swamigui_util_glade_lookup (finder->glade_widg, "LabelTime");
    gtk_label_set_text (GTK_LABEL (label), s);
    g_free (s);

    swamigui_loop_finder_update_find_button (finder, TRUE);

    results = swami_loop_finder_get_results (finder->loop_finder);  /* ++ ref */
    if (!results) return (FALSE);	/* no results? */

    matches = swami_loop_results_get_values (results, &match_count);

    gtk_list_store_clear (finder->store);	/* clear all items in store */

    if (match_count > 0)
    {
      maxq = matches[0].quality;
      diffq = matches[match_count - 1].quality - maxq;
    }

    for (i = 0; i < match_count; i++)	/* loop over result matches */
    {
      gtk_list_store_append (finder->store, &iter);

      quality = (float)(100.0 - (matches[i].quality - maxq) / diffq * 100.0);

      gtk_list_store_set (finder->store, &iter,
		  SIZE_COLUMN, matches[i].end - matches[i].start,
		  START_COLUMN, (guint)matches[i].start,
		  END_COLUMN, (guint)matches[i].end,
		  QUALITY_COLUMN, quality,
		  -1);
    }

    g_object_unref (results);	/* -- unref */

    /* -- remove reference added for worker thread (in cb_find) */
    g_object_unref (finder);

    return (FALSE);	/* monitor thread not needed anymore */
  }
  else return (TRUE);
}

/**
 * swamigui_loop_finder_new:
 *
 * Create a new sample loop finder widget.
 *
 * Returns: New widget of type SwamiguiLoopFinder
 */
GtkWidget *
swamigui_loop_finder_new (void)
{
  return (GTK_WIDGET (gtk_type_new (SWAMIGUI_TYPE_LOOP_FINDER)));
}

static gpointer
find_loop_thread (gpointer data)
{
  SwamiguiLoopFinder *finder = SWAMIGUI_LOOP_FINDER (data);
  GError *err = NULL;

  if (!swami_loop_finder_find (finder->loop_finder, &err))
  {
    g_critical (_("Find thread failed: %s"), ipatch_gerror_message (err));
    if (err) g_error_free (err);
  }

  return (NULL);
}

/**
 * swamigui_loop_finder_clear_results:
 * @finder: Loop finder GUI widget
 *
 * Clear results of a loop finder widget (if any).
 */
void
swamigui_loop_finder_clear_results (SwamiguiLoopFinder *finder)
{
  g_return_if_fail (SWAMIGUI_IS_LOOP_FINDER (finder));
  gtk_list_store_clear (finder->store);
}
