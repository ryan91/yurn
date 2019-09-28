#include <gtk/gtk.h>
#include <assert.h>
#include <math.h>

#include "yurn_style.h"

#include "yurnapp.h"
#include "yurnappwin.h"
#include "jsonparser.h"
#include "yurn_state_machine.h"

static void             yurn_app_win_init               (YurnAppWin *win);
static void             yurn_app_win_class_init         (YurnAppWinClass *class);
static YurnState        yurn_app_win_open               (gpointer data);
static YurnState        yurn_app_win_save               (gpointer data);
static YurnState        yurn_app_win_start_run          (gpointer data);
static YurnState        yurn_app_win_advance_run        (gpointer data);
static void             yurn_app_win_new_game           (YurnAppWin *win);
static void             yurn_app_adjust_splits          (YurnAppWin *win);
static GtkWidget       *yurn_app_win_get_cur_diff_lbl   (const YurnAppWin *win);
static gboolean         yurn_app_win_on_keypress        (GtkWidget *widget,
                                                         GdkEventKey *event,
                                                         gpointer data);
static gboolean         yurn_app_on_split_scroll        (GtkAdjustment *adjust,
                                                         gpointer data);
static gboolean         yurn_app_fetch_time             (gpointer data);
static GList           *yurn_app_win_get_fst_segment    (YurnAppWin *win);
static void             format_time                     (char            *buffer,
                                                         const YurnTime   time,
                                                         const gboolean   display_sign);

struct _YurnAppWin
{
  GtkApplicationWindow parent;

  struct {
    GtkWidget            *title;
    GtkWidget            *nr_tries;
    GtkWidget            *timer;
    GtkWidget            *previous_segment;
    GtkWidget            *best_possible_time;
    GtkWidget            *personal_best;
    GtkWidget            *splits;
    GtkWidget            *split_scroller;
    GtkWidget            *last_split;
    GtkWidget            *splits_box;
    GList                *split_i;
  } ui;

  struct {
    char current[16];
    char previous[16];
    char current_diff[16];
    char previous_diff[16];
  } time_model;

  struct GameData      *game;
  struct GameData      *game_backup;
  GTimer               *timer;
  char                  current_json_file[128];
  gboolean              last_split_active;
};

G_DEFINE_TYPE (YurnAppWin, yurn_app_win, GTK_TYPE_APPLICATION_WINDOW)

static inline void
add_class(GtkWidget *widget, const char *class)
{
  gtk_style_context_add_class(gtk_widget_get_style_context(widget), class);
}

static inline void
remove_class(GtkWidget *widget, const char *class)
{
  gtk_style_context_remove_class(gtk_widget_get_style_context(widget), class);
}

static YurnState
yurn_app_win_open (gpointer data)
{
  YurnAppWin  *win;
  struct GameData    *game;
  GtkWidget   *dialog;
  gint         res;
  char        *filename;
  YurnState    return_state;

  win = YURN_APP_WIN (data);
  dialog = gtk_file_chooser_dialog_new ("Open JSON", GTK_WINDOW (win),
                                        GTK_FILE_CHOOSER_ACTION_OPEN,
                                        "_Cancel", GTK_RESPONSE_CANCEL,
                                        "_Open", GTK_RESPONSE_ACCEPT, NULL);
  res = gtk_dialog_run (GTK_DIALOG (dialog));
  if (res == GTK_RESPONSE_ACCEPT)
  {
    filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
    strcpy (win->current_json_file, filename);
    game = json_parser_read_file (filename);
    if (game)
    {
      if (win->game)
      {
        game_data_free (win->game);
        game_data_free (win->game_backup);
      }
      win->game = game;
      win->game_backup = game_data_new ();
      game_data_copy (win->game_backup, game);
      yurn_app_win_new_game (win);
      return_state = YURN_STATE_GAME_LOADED;
    }
    else
    {
      // %TODO show error message
      printf ("Could not load JSON\n");
      return_state = yurn_sm_get_current_state ();
    }
    g_free (filename);
  }
  else
    return_state = yurn_sm_get_current_state ();
  gtk_widget_destroy (dialog);
  return return_state;
}

static YurnState
yurn_app_win_save (gpointer data)
{
  YurnAppWin *win;

  win = YURN_APP_WIN (data);
  printf ("Saving...\n");
  if (!json_parser_write_file (win->game, win->current_json_file))
  {
    // TODO show error message if returns false
    printf ("Could not save\n");
  }
  return yurn_sm_get_current_state ();
}

static YurnState
yurn_app_win_start_run (gpointer data)
{
  YurnAppWin *win;

  win = YURN_APP_WIN (data);
  g_timer_start (win->timer);
  add_class (GTK_WIDGET (win->ui.split_i->data), "current-split");
  return YURN_STATE_TIMER_RUNNING;
}

static YurnState
yurn_app_win_advance_run (gpointer data)
{
  YurnAppWin *win;
  GList *split_i;
  const gchar *label_txt;
  bool pb_valid;

  win = YURN_APP_WIN (data);
  pb_valid = game_data_get_current_pb (win->game) == YURN_TIME_INVALID;
  split_i = win->ui.split_i;
  game_data_advance_segment (win->game, g_timer_elapsed (win->timer, NULL));

  remove_class (GTK_WIDGET (split_i->data), "current-split");
  label_txt = gtk_label_get_text (GTK_LABEL (win->ui.timer));
  gtk_label_set_text (GTK_LABEL (win->ui.previous_segment), label_txt);
  if (pb_valid)
  {
    GList *label_row = gtk_container_get_children (GTK_CONTAINER (split_i->data));
    GtkLabel *pb = GTK_LABEL (label_row->next->next->data);
    gtk_label_set_text (pb, label_txt);
  }

  if (split_i->next)
  {
    win->ui.split_i = split_i->next;
    add_class (GTK_WIDGET (win->ui.split_i->data), "current-split");
    return YURN_STATE_TIMER_RUNNING;
  }

  return YURN_STATE_RUN_FINISHED;
}

static YurnState
yurn_app_win_pause_timer (gpointer data)
{
  YurnAppWin *win;

  win = YURN_APP_WIN (data);
  g_timer_stop (win->timer);
  return YURN_STATE_TIMER_PAUSED;
}

static YurnState
yurn_app_win_resume_timer (gpointer data)
{
  YurnAppWin *win;

  win = YURN_APP_WIN (data);
  g_timer_continue (win->timer);
  return YURN_STATE_TIMER_RUNNING;
}

static YurnState
yurn_app_win_reload (gpointer data)
{
  YurnAppWin *win;
  GTimer *timer;
  GList *segs;
  GList *segs_children;
  GList *child;
  GtkWidget *seg_i;
  YurnTime time;

  win = YURN_APP_WIN (data);
  timer = win->timer;
  time = game_data_get_pb (win->game);

  g_timer_start (timer);
  g_timer_stop (timer);
  gtk_label_set_text (GTK_LABEL (win->ui.timer), "00.0");
  for (segs = yurn_app_win_get_fst_segment (win); segs != NULL;
       segs = segs->next)
  {
    seg_i = GTK_WIDGET (segs->data);
    remove_class (seg_i, "current-split");
    segs_children = gtk_container_get_children (GTK_CONTAINER (seg_i));
    child = segs_children->next;
    gtk_label_set_text (GTK_LABEL (child->data), "");
    if (time == YURN_TIME_INVALID)
    {
      child = child->next;
      gtk_label_set_text (GTK_LABEL (child->data), "-");
    }
  }
  game_data_copy (win->game, win->game_backup);
  win->ui.split_i = yurn_app_win_get_fst_segment (win);
  return YURN_STATE_GAME_LOADED;
}

YurnAppWin *
yurn_app_win_new (YurnApp *app)
{
  return g_object_new (YURN_APP_WIN_TYPE, "application", app, NULL);
}

static void
yurn_app_win_init (YurnAppWin *win)
{
  GtkCssProvider       *win_style;
  GdkScreen            *screen;
  GdkDisplay           *display;
  GtkAdjustment        *adjust;

  win->timer = g_timer_new ();
  g_timer_stop (win->timer);
  win->game = NULL;
  win->ui.last_split = NULL;
  win->last_split_active = FALSE;

  gtk_widget_init_template (GTK_WIDGET (win));

  adjust = gtk_scrolled_window_get_vadjustment
    (GTK_SCROLLED_WINDOW (win->ui.split_scroller));

  gtk_widget_set_events (GTK_WIDGET (win), GDK_KEY_PRESS_MASK);
  g_signal_connect (G_OBJECT (win), "key_press_event",
                    G_CALLBACK (yurn_app_win_on_keypress), win);

  g_timeout_add_full (G_PRIORITY_HIGH, 50, yurn_app_fetch_time, win, NULL);

  win_style = gtk_css_provider_new ();
  display = gdk_display_get_default ();
  screen = gdk_display_get_default_screen (display);
  gtk_style_context_add_provider_for_screen
    ( screen, GTK_STYLE_PROVIDER (win_style),
      GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  gtk_css_provider_load_from_data (GTK_CSS_PROVIDER (win_style),
                                   (char *) yurn_css, yurn_css_len, NULL);

  add_class (GTK_WIDGET (win), "window");

  g_signal_connect (G_OBJECT (adjust), "value-changed",
                    G_CALLBACK (yurn_app_on_split_scroll), win);

  //////////////////////////////////////////

  yurn_sm_set_global_transition (YURN_INPUT_OPEN, yurn_app_win_open);
  yurn_sm_set_transition (YURN_STATE_GAME_LOADED, YURN_INPUT_SPACE,
                          yurn_app_win_start_run);
  yurn_sm_set_transition (YURN_STATE_TIMER_RUNNING, YURN_INPUT_F3,
                          yurn_app_win_pause_timer);
  yurn_sm_set_transition (YURN_STATE_TIMER_RUNNING, YURN_INPUT_F5,
                          yurn_app_win_reload);
  yurn_sm_set_transition (YURN_STATE_TIMER_RUNNING, YURN_INPUT_SPACE,
                          yurn_app_win_advance_run);
  yurn_sm_set_transition (YURN_STATE_TIMER_PAUSED, YURN_INPUT_F3,
                          yurn_app_win_resume_timer);
  yurn_sm_set_transition (YURN_STATE_TIMER_PAUSED, YURN_INPUT_F5,
                          yurn_app_win_reload);
  yurn_sm_set_transition (YURN_STATE_RUN_FINISHED, YURN_INPUT_F5,
                          yurn_app_win_reload);
  yurn_sm_set_transition (YURN_STATE_RUN_FINISHED, YURN_INPUT_SAVE,
                          yurn_app_win_save);
}

static void
yurn_app_win_class_init (YurnAppWinClass *class)
{
  gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (class),
                                               "/org/gtk/yurn/window.glade");
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class),
                                        YurnAppWin, ui.title);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class),
                                        YurnAppWin, ui.nr_tries);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class),
                                        YurnAppWin, ui.timer);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class),
                                        YurnAppWin, ui.previous_segment);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class),
                                        YurnAppWin, ui.best_possible_time);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class),
                                        YurnAppWin, ui.personal_best);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class),
                                        YurnAppWin, ui.splits);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class),
                                        YurnAppWin, ui.split_scroller);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class),
                                        YurnAppWin, ui.splits_box);
}

static void
yurn_app_win_new_game (YurnAppWin *win)
{
  struct GameData      *game;
  char                  buffer[16];
  GtkWidget            *splits;
  GtkWidget            *hbox;
  GtkWidget            *split_title;
  GtkWidget            *split_time;
  GtkWidget            *compare_to_pb;
  YurnTime              time;
  GList                *existing_segs;

  game = win->game;
  gtk_label_set_text (GTK_LABEL (win->ui.title), game_data_get_game_title (game));
  sprintf(buffer, "#%u", game_data_get_attempts (game));
  gtk_label_set_text (GTK_LABEL (win->ui.nr_tries), buffer);
  splits = win->ui.splits;

  for (existing_segs = gtk_container_get_children (GTK_CONTAINER (win->ui.splits));
       existing_segs != NULL; existing_segs = existing_segs->next)
    gtk_container_remove (GTK_CONTAINER (win->ui.splits),
                          GTK_WIDGET (existing_segs->data));

  for (uint8_t i = 0; i < game_data_get_nr_segments (game); ++i)
  {
    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    if (i % 2 > 0)
      add_class (hbox, "splits-odd");
    split_title = gtk_label_new (game_data_get_seg_title (game, i));
    gtk_widget_set_margin_start (split_title, 5);
    gtk_widget_set_hexpand (split_title, TRUE);
    gtk_widget_set_halign (split_title, GTK_ALIGN_START);
    compare_to_pb = gtk_label_new (NULL);
    split_time = gtk_label_new (NULL);
    gtk_widget_set_hexpand (split_time, TRUE);
    gtk_widget_set_halign (split_time, GTK_ALIGN_END);
    gtk_widget_set_margin_end (split_time, 5);
    time = game_data_get_seg_pb (game, i);
    if (time != YURN_TIME_INVALID)
    {
      format_time (buffer, time, FALSE);
      gtk_label_set_text (GTK_LABEL (split_time), buffer);
    }
    else
      gtk_label_set_text (GTK_LABEL (split_time), "-");
    gtk_container_add (GTK_CONTAINER (hbox), split_title);
    gtk_container_add (GTK_CONTAINER (hbox), compare_to_pb);
    gtk_container_add (GTK_CONTAINER (hbox), split_time);
    gtk_container_add (GTK_CONTAINER (splits), hbox);
    if (i == game_data_get_nr_segments (game) - 1)
    {
      g_object_ref (hbox);
      win->ui.last_split = hbox;
    }
  }
  win->ui.split_i = yurn_app_win_get_fst_segment (win);

  time = game_data_get_sum_of_best_segs (game);

  if (time != YURN_TIME_INVALID)
  {
    format_time (buffer, game_data_get_sum_of_best_segs (game), FALSE);
    gtk_label_set_text(GTK_LABEL (win->ui.best_possible_time), buffer);
  }

  gtk_widget_show_all (win->ui.splits);
}

static void
yurn_app_adjust_splits (YurnAppWin *win)
{
  gint                  dest_y;
  GtkScrolledWindow    *scroll;
  GtkAdjustment        *adjust;
  GtkWidget            *cur_seg;
  int                   scroll_height;
  int                   cur_height;

  cur_seg = GTK_WIDGET (win->ui.split_i->data);
  scroll = GTK_SCROLLED_WINDOW (win->ui.split_scroller);
  adjust = gtk_scrolled_window_get_vadjustment (scroll);
  gtk_widget_translate_coordinates (win->ui.splits,
                                    cur_seg,
                                    0, 0, NULL, &dest_y);
  dest_y = abs(dest_y);
  scroll_height = gtk_widget_get_allocated_height (GTK_WIDGET (scroll));
  cur_height = gtk_widget_get_allocated_height (cur_seg);

  if (cur_height + dest_y > scroll_height)
    gtk_adjustment_set_value (adjust, dest_y + cur_height - scroll_height);
}

static GtkWidget *
yurn_app_win_get_cur_diff_lbl (const YurnAppWin *win)
{
  GList                *children;

  assert (win->ui.split_i);
  children = gtk_container_get_children (win->ui.split_i->data);

  return children->next->data;
}

static gboolean
yurn_app_win_on_keypress (GtkWidget *widget, GdkEventKey *event, gpointer data)
{
  switch (event->keyval)
  {
    case GDK_KEY_space: yurn_sm_transition (YURN_INPUT_SPACE, data); break;
    case GDK_KEY_F3:    yurn_sm_transition (YURN_INPUT_F3, data); break;
    case GDK_KEY_F5:    yurn_sm_transition (YURN_INPUT_F5, data); break;
    default: return FALSE;
  }

  return TRUE;
}

static gboolean
yurn_app_on_split_scroll (GtkAdjustment *adjust, gpointer data)
{
  YurnAppWin           *win;
  GtkContainer         *splits;
  GtkContainer         *splits_box;
  GtkWidget            *last_split;
  double                current_scroll;
  double                max_scroll;
  double                page_size;
  gboolean              last_split_active;

  win               = YURN_APP_WIN (data);

  if (!win->game)
    return TRUE;

  current_scroll    = gtk_adjustment_get_value (adjust);
  max_scroll        = gtk_adjustment_get_upper (adjust);
  page_size         = gtk_adjustment_get_page_size (adjust);
  splits            = GTK_CONTAINER (win->ui.splits);
  splits_box        = GTK_CONTAINER (win->ui.splits_box);
  last_split        = win->ui.last_split;
  last_split_active = win->last_split_active;

  if (current_scroll + page_size < max_scroll)
  {
    if (!last_split_active)
    {
      gtk_container_remove (splits, last_split);
      gtk_container_add (splits_box, last_split);
      win->last_split_active = TRUE;
    }
  }
  else
  {
    if (last_split_active)
    {
      gtk_container_remove (splits_box, last_split);
      gtk_container_add (splits, last_split);
      win->last_split_active = FALSE;
    }
  }
  return TRUE;
}

static gboolean
yurn_app_fetch_time (gpointer data)
{
  YurnTime                seconds;
  YurnAppWin             *win;
  GtkLabel               *diff_lbl;
  struct GameData        *game;
  bool                    classification_changed;

  if (yurn_sm_get_current_state () != YURN_STATE_TIMER_RUNNING)
    return TRUE;

  win = YURN_APP_WIN (data);

  char *current_time = win->time_model.current;
  char *previous_time = win->time_model.previous;
  char *current_diff = win->time_model.current_diff;
  char *previous_diff = win->time_model.previous_diff;

  diff_lbl = GTK_LABEL (yurn_app_win_get_cur_diff_lbl (win));
  seconds = g_timer_elapsed (win->timer, NULL);
  game = win->game;
  classification_changed = game_data_set_current_time (game, seconds);

  // set label text of large timer
  format_time (current_time, seconds, FALSE);
  if (strcmp (current_time, previous_time))
  {
    gtk_label_set_text (GTK_LABEL (win->ui.timer), current_time);
    strcpy (previous_time, current_time);
  }

  // set class of large timer
  if (classification_changed)
  {
    printf ("Classification changed (%d)\n", game_data_get_classification (game));
    switch (game_data_get_classification (game))
    {
      case TIME_CLASS_BETTER_THAN_BEST_SEG:
      case TIME_CLASS_BETTER_THAN_PB:
      case TIME_CLASS_INVALID:
        add_class (win->ui.timer, "split-green"); break;
      case TIME_CLASS_WORSE_THAN_PB:
        add_class (win->ui.timer, "split-red"); break;
      case TIME_CLASS_LOSING:
        add_class (win->ui.timer, "split-losing"); break;
    }
  }

  if (game_data_get_classification (game) == TIME_CLASS_INVALID)
    return TRUE;

  // set label text of diff timer
  format_time (current_diff, game_data_get_segment_time (game), TRUE);
  if (strcmp (current_diff, previous_diff))
  {
      gtk_label_set_text (diff_lbl, current_diff);
      strcpy (previous_diff, current_diff);
  }

  // set class of diff timer
  if (classification_changed)
  {
    switch (game_data_get_classification (game))
    {
      case TIME_CLASS_BETTER_THAN_BEST_SEG:
        add_class (GTK_WIDGET (diff_lbl), "split-gold"); break;
      case TIME_CLASS_BETTER_THAN_PB:
      case TIME_CLASS_INVALID:
        add_class (GTK_WIDGET (diff_lbl), "split-green"); break;
      case TIME_CLASS_WORSE_THAN_PB:
        add_class (GTK_WIDGET (diff_lbl), "split-red"); break;
      case TIME_CLASS_LOSING:
        add_class (GTK_WIDGET (diff_lbl), "split-losing"); break;
    }
  }

  return TRUE;
}

static void
format_time (char            *buffer,
             const YurnTime   time,
             const gboolean   display_sign)
{
  YurnTime              seconds;
  char                  hours;
  char                  minutes;
  gboolean              is_positive;

  seconds = time;
  is_positive = seconds >= 0;
  seconds = fabs (seconds);

  hours = seconds / 3600;
  seconds -= 3600 * hours;
  minutes = seconds / 60;
  seconds -= 60 * minutes;

  if (display_sign)
  {
    if (is_positive)
    {
      if (hours && minutes)
        sprintf (buffer, "+%d:%d:%04.1f", hours, minutes, seconds);
      else if (minutes)
        sprintf (buffer, "+%d:%04.1f", minutes, seconds);
      else
        sprintf (buffer, "+%04.1f", seconds);
    }
    else
    {
      if (hours && minutes)
        sprintf (buffer, "-%d:%d:%04.1f", hours, minutes, seconds);
      else if (minutes)
        sprintf (buffer, "-%d:%04.1f", minutes, seconds);
      else
        sprintf (buffer, "-%04.1f", seconds);
    }
  }
  else
  {
    if (hours && minutes)
      sprintf (buffer, "%d:%d:%04.1f", hours, minutes, seconds);
    else if (minutes)
      sprintf (buffer, "%d:%04.1f", minutes, seconds);
    else
      sprintf (buffer, "%04.1f", seconds);
  }
}

static GList *
yurn_app_win_get_fst_segment (YurnAppWin *win)
{
  return gtk_container_get_children (GTK_CONTAINER (win->ui.splits));
}
