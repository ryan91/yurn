#include <gtk/gtk.h>
#include <assert.h>
#include <math.h>

#include "yurn_style.h"

#include "exampleapp.h"
#include "exampleappwin.h"
#include "jsonparser.h"

static void
example_app_window_reload_game (ExampleAppWindow *win);

typedef enum _TimerState
{
  TIMER_STARTED,
  TIMER_PAUSED,
  TIMER_STOPPED,
  TIMER_FINISHED
} TimerState;

struct _ExampleAppWindow
{
  GtkApplicationWindow parent;

  GtkWidget *title;
  GtkWidget *nr_tries;
  GtkWidget *main_timer;
  GtkWidget *previous_segment;
  GtkWidget *best_possible_time;
  GtkWidget *personal_best;
  GtkWidget *splits;
  GtkWidget *split_scroller;
  GtkWidget *last_split;
  GtkWidget *splits_box;
  Segment   **current_segment;

  GameData *game;
  GTimer *timer;
  char current_time[16];
  char old_time[16];
  char current_diff_to_pb[16];
  char old_diff_to_pb[16];
  TimerState timer_state;
  GList *segments;
  gboolean last_split_active;
};

G_DEFINE_TYPE (ExampleAppWindow, example_app_window, GTK_TYPE_APPLICATION_WINDOW)

static inline void
add_class(GtkWidget *widget, const char *class) {
  gtk_style_context_add_class(gtk_widget_get_style_context(widget), class);
}

static inline void
remove_class(GtkWidget *widget, const char *class) {
  gtk_style_context_remove_class(gtk_widget_get_style_context(widget), class);
}

static GtkWidget *
example_app_window_get_cur_diff_lbl (const ExampleAppWindow *win)
{
  GList *children;
  assert (win->segments);

  children = gtk_container_get_children (win->segments->data);
  return children->next->data;
}

static void
example_app_window_clock_reset (ExampleAppWindow *win)
{
  GTimer *timer;

  timer = win->timer;
  gtk_label_set_text (GTK_LABEL (win->main_timer), "00.0");
  g_timer_start (timer);
  g_timer_stop (timer);
  win->timer_state = TIMER_STOPPED;
}

static void
example_app_window_clock_start (ExampleAppWindow *win)
{
  g_timer_start (win->timer);
  win->timer_state = TIMER_STARTED;
}

static void
example_app_window_clock_stop (ExampleAppWindow *win)
{
  g_timer_stop (win->timer);
  win->timer_state = TIMER_PAUSED;
}

static void
example_app_window_clock_resume (ExampleAppWindow *win)
{
  g_timer_continue (win->timer);
  win->timer_state = TIMER_STARTED;
}

static void
example_app_window_split_start (ExampleAppWindow *win)
{
  assert (win->segments->data);

  add_class (GTK_WIDGET (win->segments->data), "current-split");
}

static void example_app_set_previous_segment (ExampleAppWindow *win)
{
  GtkWidget *diff_lbl;
  GtkStyleContext *context;
  const gchar split_css[4][12] =
    { "split-gold", "split-green", "split-red", "split-losing" };

  diff_lbl = example_app_window_get_cur_diff_lbl (win);
  gtk_label_set_text (GTK_LABEL (win->previous_segment),
                      gtk_label_get_text (GTK_LABEL (diff_lbl)));
  context = gtk_widget_get_style_context (diff_lbl);
  for (int i = 0; i < 4; ++i)
    remove_class (win->previous_segment, split_css[i]);
  for (int i = 0; i < 4; ++i)
    if (gtk_style_context_has_class (context, split_css[i]))
      add_class (win->previous_segment, split_css[i]);
}

static void
example_app_window_split_step (ExampleAppWindow *win)
{
  GList *segs;

  segs = win->segments;
  example_app_set_previous_segment (win);
  if (segs->next)
  {

    remove_class (GTK_WIDGET (win->segments->data), "current-split");
    win->segments = win->segments->next;
    add_class (GTK_WIDGET (win->segments->data), "current-split");
    ++(win->current_segment);
  }
  else if (segs)
  {
    remove_class (GTK_WIDGET (win->segments->data), "current-split");
    win->timer_state = TIMER_FINISHED;
  }
}

static void
example_app_window_split_reset (ExampleAppWindow *win)
{
  GList *segs;

  segs = win->segments;
  for (segs = gtk_container_get_children(GTK_CONTAINER (win->splits));
       segs != NULL; segs = segs->next)
    remove_class (GTK_WIDGET (segs->data), "current-split");
  win->segments = gtk_container_get_children (GTK_CONTAINER (win->splits));
  win->current_segment = win->game->segments;
}

static void
format_time (char *buffer,
             const YurnTime time,
             const gboolean display_sign)
{
  YurnTime seconds = time;
  char hours, minutes;
  char format_str[32];
  char *ci;

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

static gboolean
handle_keypress (GtkWidget *widget, GdkEventKey *event, gpointer data)
{
  ExampleAppWindow *win;
  TimerState state;

  win = EXAMPLE_APP_WINDOW (data);
  state = win->timer_state;

  if (!win->game)
  {
    return FALSE;
  }

  switch (event->keyval)
  {
    case GDK_KEY_space:
      switch (state)
      {
        case TIMER_STARTED:
          example_app_window_split_step (win);
          return TRUE;
        case TIMER_STOPPED:
          example_app_window_clock_start (win);
          example_app_window_split_start (win);
          return TRUE;
        case TIMER_PAUSED:
          return TRUE;
        case TIMER_FINISHED:
          return TRUE;
      }
    case GDK_KEY_F3:
      if (win->timer_state == TIMER_STARTED)
        example_app_window_clock_stop (win);
      else
        example_app_window_clock_resume (win);
      return TRUE;
    case GDK_KEY_F5:
      example_app_window_reload_game (win);
      return TRUE;
    default:
      return FALSE;
  }
}

static gboolean
poll_time (gpointer data)
{
  double seconds;
  double diff_to_current_segment;
  double diff_to_best_seg;
  ExampleAppWindow *win;
  GtkLabel *diff_lbl;

  win = EXAMPLE_APP_WINDOW (data);

  if (win->timer_state != TIMER_STARTED)
    return TRUE;

  seconds = g_timer_elapsed (win->timer, NULL);

  diff_to_current_segment = seconds - (*(win->current_segment))->pb_run;
  diff_to_best_seg = seconds - (*(win->current_segment))->best_seg;

  format_time (win->current_time, seconds, FALSE);

  if (strcmp (win->current_time, win->old_time))
  {
    gtk_label_set_text (GTK_LABEL (win->main_timer), win->current_time);
    if (diff_to_current_segment < 0)
      remove_class (GTK_WIDGET (win->main_timer), "timer-red");
    else
      add_class (GTK_WIDGET (win->main_timer), "timer-red");
    strcpy (win->old_time, win->current_time);
  }

  if (diff_to_best_seg > -15.)
  {
    format_time (win->current_diff_to_pb, diff_to_current_segment, TRUE);
    if (strcmp (win->current_diff_to_pb, win->old_diff_to_pb))
    {
      diff_lbl = GTK_LABEL (example_app_window_get_cur_diff_lbl (win));
      gtk_label_set_text (diff_lbl, win->current_diff_to_pb);
      strcpy (win->old_diff_to_pb, win->current_diff_to_pb);

      if (diff_to_best_seg < 0)
        add_class (GTK_WIDGET (diff_lbl), "split-gold");
      else if (diff_to_current_segment < 0)
        add_class (GTK_WIDGET (diff_lbl), "split-green");
      else
        add_class (GTK_WIDGET (diff_lbl), "split-red");
      // TODO split-losing
    }
  }

  return TRUE;
}

static gboolean
handle_scroller_change (GtkAdjustment *adjust, gpointer data)
{
  ExampleAppWindow               *win;
  GtkContainer                   *splits;
  GtkContainer                   *splits_box;
  GtkWidget                      *last_split;
  double                         current_scroll;
  double                         max_scroll;
  double                         page_size;
  gboolean                       last_split_active;

  win            = EXAMPLE_APP_WINDOW (data);

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

static void
example_app_window_init (ExampleAppWindow *win)
{
  GtkCssProvider *win_style;
  GdkScreen *screen;
  GdkDisplay *display;
  GtkAdjustment *adjust;

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
  g_signal_connect (G_OBJECT (win), "key_press_event", G_CALLBACK (handle_keypress), win);

  g_timeout_add_full (G_PRIORITY_HIGH, 50, poll_time, win, NULL);

  win_style = gtk_css_provider_new ();
  display = gdk_display_get_default ();
  screen = gdk_display_get_default_screen (display);
  gtk_style_context_add_provider_for_screen (
      screen,
      GTK_STYLE_PROVIDER(win_style),
      GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  gtk_css_provider_load_from_data (GTK_CSS_PROVIDER (win_style),
                                   (char *) yurn_css, yurn_css_len, NULL);

  add_class (GTK_WIDGET (win), "window");

  g_signal_connect (G_OBJECT (adjust), "value-changed", G_CALLBACK (handle_scroller_change), win);
}

static void
example_app_window_class_init (ExampleAppWindowClass *class)
{
  gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (class),
                                               "/org/gtk/exampleapp/window.glade");
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class),
                                        ExampleAppWindow, title);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class),
                                        ExampleAppWindow, nr_tries);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class),
                                        ExampleAppWindow, main_timer);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class),
                                        ExampleAppWindow, previous_segment);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class),
                                        ExampleAppWindow, best_possible_time);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class),
                                        ExampleAppWindow, personal_best);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class),
                                        ExampleAppWindow, splits);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class),
                                        ExampleAppWindow, split_scroller);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class),
                                        ExampleAppWindow, splits_box);
}

static void
example_app_window_reload_game (ExampleAppWindow *win)
{
  assert (win->game);

  // order is important here!
  example_app_window_clock_reset (win);
  example_app_window_split_reset (win);
}

static void
example_app_window_new_game (ExampleAppWindow *win)
{
  GameData *game;
  char buffer[16];
  GtkWidget *splits;
  GtkWidget *hbox;
  // GtkWidget *expand_box;
  GtkWidget *split_title;
  GtkWidget *split_time;
  GtkWidget *compare_to_pb;
  Segment *seg;
  YurnTime sum_of_best_segments;

  game = win->game;
  gtk_label_set_text (GTK_LABEL (win->title), game->title);
  sprintf(buffer, "#%u", game->attempts);
  gtk_label_set_text (GTK_LABEL (win->nr_tries), buffer);
  splits = win->splits;

  for (uint8_t i = 0; i < game->nr_segments; ++i) {
    seg = game->segments[i];
    // expand_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    // gtk_widget_set_hexpand (expand_box, TRUE);
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
    } else
    {
      gtk_label_set_text (GTK_LABEL (split_time), "-");
    }
    gtk_container_add (GTK_CONTAINER (hbox), split_title);
    // gtk_container_add (GTK_CONTAINER (hbox), expand_box);
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
  {
    sum_of_best_segments += game->segments[i]->best_seg;
  }
  format_time (buffer, sum_of_best_segments, FALSE);
  gtk_label_set_text(GTK_LABEL (win->best_possible_time), buffer);

  gtk_widget_show_all (win->splits);
}

ExampleAppWindow *
example_app_window_new (ExampleApp *app)
{
  return g_object_new (EXAMPLE_APP_WINDOW_TYPE, "application", app, NULL);
}

void
example_app_window_open (ExampleAppWindow *win,
                         const char       *file)
{
  GameData *game = json_parser_read_file(file);
  // TODO unload old game if existed
  win->game = game;
  if (game)
  {
    example_app_window_new_game (win);
  }
}
