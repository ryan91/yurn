#include <gtk/gtk.h>
#include <assert.h>
#include <math.h>

#include "yurn_style.h"

#include "yurnapp.h"
#include "yurnappwin.h"
#include "jsonparser.h"

static void             yurn_app_win_init               (YurnAppWin *win);
static void             yurn_app_win_class_init         (YurnAppWinClass *class);
static void             yurn_app_win_new_game           (YurnAppWin *win);
static void             yurn_app_win_reload_game        (YurnAppWin *win);
static void             yurn_app_win_clock_start        (YurnAppWin *win);
static void             yurn_app_win_clock_stop         (YurnAppWin *win);
static void             yurn_app_win_clock_resume       (YurnAppWin *win);
static void             yurn_app_win_clock_reset        (YurnAppWin *win);
static void             yurn_app_win_split_start        (YurnAppWin *win);
static void             yurn_app_win_split_step         (YurnAppWin *win);
static void             yurn_app_win_split_reset        (YurnAppWin *win);
static void             yurn_app_win_prev_seg_set       (YurnAppWin *win);
static void             yurn_app_win_prev_seg_reset     (YurnAppWin *win);
static void             yurn_app_adjust_splits          (YurnAppWin *win);
static void             yurn_app_win_calc_best_segs     (YurnAppWin *win);
static void             yurn_app_take_timestamp         (YurnAppWin *win,
                                                         double     *ts);
static GtkWidget       *yurn_app_win_get_cur_diff_lbl   (const YurnAppWin *win);
static gboolean         yurn_app_win_on_keypress        (GtkWidget *widget,
                                                         GdkEventKey *event,
                                                         gpointer data);
static gboolean         yurn_app_on_split_scroll        (GtkAdjustment *adjust,
                                                         gpointer data);
static gboolean         yurn_app_fetch_time             (gpointer data);
static void             format_time                     (char            *buffer,
                                                         const YurnTime   time,
                                                         const gboolean   display_sign);

typedef enum _YurnState
{
  YURN_STATE_INITIAL,
  YURN_STATE_GAME_LOADED,
  YURN_STATE_TIMER_RUNNING,
  YURN_STATE_TIMER_PAUSED,
  YURN_STATE_RUN_FINISHED
} YurnState;

typedef enum _YurnInput
{
  YURN_INPUT_SPACE,
  YURN_INPUT_F5,
  YURN_INPUT_F3,
  YURN_INPUT_OPEN,
  YURN_INPUT_SAVE,
  YURN_INPUT_RELOAD,
  YURN_INPUT_QUIT
} YurnInput;

typedef enum _TimerState
{
  TIMER_STARTED,
  TIMER_PAUSED,
  TIMER_STOPPED,
  TIMER_FINISHED

} TimerState;

struct _YurnAppWin
{
  GtkApplicationWindow parent;

  GtkWidget            *title;
  GtkWidget            *nr_tries;
  GtkWidget            *main_timer;
  GtkWidget            *previous_segment;
  GtkWidget            *best_possible_time;
  GtkWidget            *personal_best;
  GtkWidget            *splits;
  GtkWidget            *split_scroller;
  GtkWidget            *last_split;
  GtkWidget            *splits_box;
  Segment             **current_segment;

  GameData             *game;
  GTimer               *timer;
  char                  current_time[16];
  char                  old_time[16];
  char                  current_diff_to_pb[16];
  char                  old_diff_to_pb[16];
  TimerState            timer_state;
  GList                *segments;
  gboolean              last_split_active;
  YurnTime              sum_of_remaining_splits;
  YurnTime              seg_start_timestamp;
};

G_DEFINE_TYPE (YurnAppWin, yurn_app_win, GTK_TYPE_APPLICATION_WINDOW)

YurnAppWin *
yurn_app_win_new (YurnApp *app)
{
  return g_object_new (YURN_APP_WIN_TYPE, "application", app, NULL);
}

void
yurn_app_win_open (YurnAppWin *win,
                   const char *file)
{
  GameData             *game;
  
  game = json_parser_read_file(file);

  // TODO unload old game if existed

  win->game = game;
  if (game)
    yurn_app_win_new_game (win);
}

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
  win->timer_state = TIMER_STOPPED;
  win->current_segment = NULL;
  win->last_split = NULL;
  win->last_split_active = FALSE;

  gtk_widget_init_template (GTK_WIDGET (win));

  adjust = gtk_scrolled_window_get_vadjustment
    (GTK_SCROLLED_WINDOW (win->split_scroller));

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
}

static void
yurn_app_win_class_init (YurnAppWinClass *class)
{
  gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (class),
                                               "/org/gtk/yurn/window.glade");
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class),
                                        YurnAppWin, title);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class),
                                        YurnAppWin, nr_tries);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class),
                                        YurnAppWin, main_timer);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class),
                                        YurnAppWin, previous_segment);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class),
                                        YurnAppWin, best_possible_time);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class),
                                        YurnAppWin, personal_best);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class),
                                        YurnAppWin, splits);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class),
                                        YurnAppWin, split_scroller);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class),
                                        YurnAppWin, splits_box);
}

static void
yurn_app_win_new_game (YurnAppWin *win)
{
  GameData             *game;
  char                  buffer[16];
  GtkWidget            *splits;
  GtkWidget            *hbox;
  GtkWidget            *split_title;
  GtkWidget            *split_time;
  GtkWidget            *compare_to_pb;
  Segment              *seg;
  YurnTime              sum_of_best_segments;

  game = win->game;
  gtk_label_set_text (GTK_LABEL (win->title), game->title);
  sprintf(buffer, "#%u", game->attempts);
  gtk_label_set_text (GTK_LABEL (win->nr_tries), buffer);
  splits = win->splits;

  for (uint8_t i = 0; i < game->nr_segments; ++i)
  {
    seg = game->segments[i];
    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    if (i % 2 > 0)
      add_class (hbox, "splits-odd");
    split_title = gtk_label_new (seg->title);
    gtk_widget_set_margin_start (split_title, 5);
    gtk_widget_set_hexpand (split_title, TRUE);
    gtk_widget_set_halign (split_title, GTK_ALIGN_START);
    compare_to_pb = gtk_label_new (NULL);
    split_time = gtk_label_new (NULL);
    gtk_widget_set_hexpand (split_time, TRUE);
    gtk_widget_set_halign (split_time, GTK_ALIGN_END);
    gtk_widget_set_margin_end (split_time, 5);
    if (seg->pb_run)
    {
      format_time (buffer, seg->pb_run, FALSE);
      gtk_label_set_text (GTK_LABEL (split_time), buffer);
    }
    else
      gtk_label_set_text (GTK_LABEL (split_time), "-");
    gtk_container_add (GTK_CONTAINER (hbox), split_title);
    gtk_container_add (GTK_CONTAINER (hbox), compare_to_pb);
    gtk_container_add (GTK_CONTAINER (hbox), split_time);
    gtk_container_add (GTK_CONTAINER (splits), hbox);
    if (i == game->nr_segments - 1)
    {
      g_object_ref (hbox);
      win->last_split = hbox;
    }
  }
  win->segments = gtk_container_get_children (GTK_CONTAINER (win->splits));
  win->current_segment = win->game->segments;

  sum_of_best_segments = 0.;
  for (uint8_t i = 0; i < game->nr_segments; ++i)
    sum_of_best_segments += game->segments[i]->best_seg;
  format_time (buffer, sum_of_best_segments, FALSE);
  gtk_label_set_text(GTK_LABEL (win->best_possible_time), buffer);

  gtk_widget_show_all (win->splits);
}

static void
yurn_app_win_reload_game (YurnAppWin *win)
{
  assert (win->game);

  // order is important here!
  yurn_app_win_clock_reset (win);
  yurn_app_win_split_reset (win);
  yurn_app_win_prev_seg_reset (win);
}

static void
yurn_app_win_clock_start (YurnAppWin *win)
{
  g_timer_start (win->timer);
  win->timer_state = TIMER_STARTED;
}

static void
yurn_app_win_clock_stop (YurnAppWin *win)
{
  g_timer_stop (win->timer);
  win->timer_state = TIMER_PAUSED;
}

static void
yurn_app_win_clock_resume (YurnAppWin *win)
{
  g_timer_continue (win->timer);
  win->timer_state = TIMER_STARTED;
}

static void
yurn_app_win_clock_reset (YurnAppWin *win)
{
  GTimer               *timer;

  timer = win->timer;
  gtk_label_set_text (GTK_LABEL (win->main_timer), "00.0");
  g_timer_start (timer);
  g_timer_stop (timer);
  win->timer_state = TIMER_STOPPED;
}

static void
yurn_app_win_split_start (YurnAppWin *win)
{
  assert (win->segments->data);

  add_class (GTK_WIDGET (win->segments->data), "current-split");
  yurn_app_win_calc_best_segs (win);
  win->seg_start_timestamp = 0;
}

static void
yurn_app_win_split_step (YurnAppWin *win)
{
  GList                *segs;
  Segment              *cur_seg;
  YurnTime              seconds;

  segs = win->segments;
  cur_seg = *win->current_segment;
  yurn_app_take_timestamp (win, &seconds);

  if (cur_seg->pb_run > seconds)
    cur_seg->pb_run = seconds;
  if (cur_seg->best_seg > seconds - win->seg_start_timestamp)
    cur_seg->best_seg = seconds - win->seg_start_timestamp;
  yurn_app_win_prev_seg_set (win);
  if (segs->next)
  {
    remove_class (GTK_WIDGET (win->segments->data), "current-split");
    win->segments = win->segments->next;
    add_class (GTK_WIDGET (win->segments->data), "current-split");
    ++(win->current_segment);
    yurn_app_adjust_splits (win);
  }
  else if (segs)
  {
    remove_class (GTK_WIDGET (win->segments->data), "current-split");
    win->timer_state = TIMER_FINISHED;
  }
  yurn_app_win_calc_best_segs (win);
}

static void
yurn_app_win_split_reset (YurnAppWin *win)
{
  GList                *segs;
  GtkWidget            *seg;
  GList                *seg_children;
  GtkLabel             *diff_lbl;

  segs = win->segments;
  for (segs = gtk_container_get_children(GTK_CONTAINER (win->splits));
       segs != NULL; segs = segs->next)
  {
    seg = GTK_WIDGET (segs->data);
    remove_class (seg, "current-split");
    seg_children = gtk_container_get_children (GTK_CONTAINER (seg));
    diff_lbl = GTK_LABEL (seg_children->next->data);
    gtk_label_set_text (diff_lbl, "");
  }
  win->segments = gtk_container_get_children (GTK_CONTAINER (win->splits));
  win->current_segment = win->game->segments;
}

static void yurn_app_win_prev_seg_set (YurnAppWin *win)
{
  GtkWidget            *diff_lbl;
  GtkStyleContext      *context;
  const gchar           split_css[4][12] =
    { "split-gold", "split-green", "split-red", "split-losing" };

  diff_lbl = yurn_app_win_get_cur_diff_lbl (win);
  gtk_label_set_text (GTK_LABEL (win->previous_segment),
                      gtk_label_get_text (GTK_LABEL (diff_lbl)));
  context = gtk_widget_get_style_context (diff_lbl);
  for (int i = 0; i < 4; ++i)
    remove_class (win->previous_segment, split_css[i]);
  for (int i = 0; i < 4; ++i)
    if (gtk_style_context_has_class (context, split_css[i]))
      add_class (win->previous_segment, split_css[i]);
}

static void yurn_app_win_prev_seg_reset (YurnAppWin *win)
{
  gtk_label_set_text (GTK_LABEL (win->previous_segment), "");
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

  scroll = GTK_SCROLLED_WINDOW (win->split_scroller);
  adjust = gtk_scrolled_window_get_vadjustment (scroll);
  gtk_widget_translate_coordinates (win->splits,
                                    GTK_WIDGET (win->segments->data),
                                    0, 0, NULL, &dest_y);
  dest_y = abs(dest_y);
  cur_seg = GTK_WIDGET (win->segments->data);
  scroll_height = gtk_widget_get_allocated_height (GTK_WIDGET (scroll));
  cur_height = gtk_widget_get_allocated_height (cur_seg);

  if (cur_height + dest_y > scroll_height)
    gtk_adjustment_set_value (adjust, dest_y + cur_height - scroll_height);
}

static void
yurn_app_win_calc_best_segs (YurnAppWin *win)
{
  Segment             **iter;
  YurnTime              sum;

  sum = 0;
  for (iter = win->current_segment + 1; *iter != NULL; ++iter)
  {
    if (!(*iter)->best_seg)
    {
      win->sum_of_remaining_splits = -1.;
      return;
    }
    sum += (*iter)->best_seg;
  }
  win->sum_of_remaining_splits = sum;
}

static void
yurn_app_take_timestamp (YurnAppWin *win, double *ts)
{
  *ts = g_timer_elapsed (win->timer, NULL);
}

static GtkWidget *
yurn_app_win_get_cur_diff_lbl (const YurnAppWin *win)
{
  GList                *children;

  assert (win->segments);
  children = gtk_container_get_children (win->segments->data);

  return children->next->data;
}

static gboolean
yurn_app_win_on_keypress (GtkWidget *widget, GdkEventKey *event, gpointer data)
{
  YurnAppWin           *win;
  TimerState            state;

  win = YURN_APP_WIN (data);
  state = win->timer_state;

  if (!win->game)
    return FALSE;

  switch (event->keyval)
  {
    case GDK_KEY_space:
      switch (state)
      {
        case TIMER_STARTED:
          yurn_app_win_split_step (win);
          return TRUE;
        case TIMER_STOPPED:
          yurn_app_win_clock_start (win);
          yurn_app_win_split_start (win);
          return TRUE;
        case TIMER_PAUSED:
          return TRUE;
        case TIMER_FINISHED:
          return TRUE;
      }
    case GDK_KEY_F3:
      if (win->timer_state == TIMER_STARTED)
        yurn_app_win_clock_stop (win);
      else
        yurn_app_win_clock_resume (win);
      return TRUE;
    case GDK_KEY_F5:
      yurn_app_win_reload_game (win);
      return TRUE;
    default:
      return FALSE;
  }
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
  splits            = GTK_CONTAINER (win->splits);
  splits_box        = GTK_CONTAINER (win->splits_box);
  last_split        = win->last_split;
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
  double                seconds;
  double                diff_to_current_segment;
  double                diff_to_best_seg;
  YurnAppWin           *win;
  GtkLabel             *diff_lbl;
  const GameData       *game;

  win = YURN_APP_WIN (data);

  if (win->timer_state != TIMER_STARTED)
    return TRUE;

  yurn_app_take_timestamp (win, &seconds);
  game    = win->game;

  diff_to_current_segment = seconds - (*(win->current_segment))->pb_run;
  diff_to_best_seg = seconds - (*(win->current_segment))->best_seg;

  format_time (win->current_time, seconds, FALSE);

  if (strcmp (win->current_time, win->old_time))
  {

    gtk_label_set_text (GTK_LABEL (win->main_timer), win->current_time);
    if (!(*(win->current_segment))->best_seg ||
        !(*(win->current_segment))->pb_run ||
        diff_to_current_segment < 0)
    {
      remove_class (GTK_WIDGET (win->main_timer), "timer-red");
      remove_class (GTK_WIDGET (win->main_timer), "timer-losing");
    }
    else
    {
      if (seconds + win->sum_of_remaining_splits >
          game->segments[game->nr_segments - 1]->pb_run)
        add_class (GTK_WIDGET (win->main_timer), "split-losing");
      else
        add_class (GTK_WIDGET (win->main_timer), "split-red");
    }
    strcpy (win->old_time, win->current_time);
  }

  if (!(*(win->current_segment))->best_seg ||
      !(*(win->current_segment))->pb_run)
    return TRUE;

  if (diff_to_best_seg > -15.)
  {
    format_time (win->current_diff_to_pb, diff_to_current_segment, TRUE);
    if (strcmp (win->current_diff_to_pb, win->old_diff_to_pb))
    {
        
      diff_lbl = GTK_LABEL (yurn_app_win_get_cur_diff_lbl (win));
      gtk_label_set_text (diff_lbl, win->current_diff_to_pb);
      strcpy (win->old_diff_to_pb, win->current_diff_to_pb);

      if (diff_to_best_seg < 0)
        add_class (GTK_WIDGET (diff_lbl), "split-gold");
      else if (diff_to_current_segment < 0)
        add_class (GTK_WIDGET (diff_lbl), "split-green");
      else if (diff_to_current_segment < 60.)
        add_class (GTK_WIDGET (diff_lbl), "split-red");
      else
        add_class (GTK_WIDGET (diff_lbl), "split-losing");
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
  char                  format_str[32];
  char                 *ci;

  seconds = time;
  hours = seconds / 3600;
  seconds -= 3600 * hours;
  minutes = seconds / 60;
  seconds -= 60 * minutes;

  ci = format_str;
  *ci = '%';
  ++ci;
  if (display_sign)
  {
    *ci = '+';
    ++ci;
  }
  if (hours)
  {
    strcpy (ci, "02d:%");
    ci += 5;
    minutes = abs (minutes);
    seconds = fabs (seconds);
  }
  if (hours || minutes)
  {
    strcpy (ci, "02d:%02d");
    ci += 8;
    if (!hours)
      seconds = fabs (seconds);
  }
  else
  {
    strcpy (ci, "04.1f");
    ci += 5;
  }

  *ci = 0; // 0-terminated

  if (hours)
    sprintf (buffer, format_str, hours, minutes, (int) seconds);
  else if (minutes)
    sprintf (buffer, format_str, minutes, (int) seconds);
  else
    sprintf (buffer, format_str, seconds);
}
