#include <gtk/gtk.h>
#include <assert.h>

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

  GameData *game;
  GTimer *timer;
  char current_time[16];
  char old_time[16];
  TimerState timer_state;
  GList *segments;
};

G_DEFINE_TYPE (ExampleAppWindow, example_app_window, GTK_TYPE_APPLICATION_WINDOW)

typedef enum _FormatTimeType
{
  format_split,
  format_difftime,
  format_best_times
} FormatTimeType;

static inline void
add_class(GtkWidget *widget, const char *class) {
  gtk_style_context_add_class(gtk_widget_get_style_context(widget), class);
}

static inline void
remove_class(GtkWidget *widget, const char *class) {
  gtk_style_context_remove_class(gtk_widget_get_style_context(widget), class);
}

static void
example_app_window_clock_reset (ExampleAppWindow *win)
{
  GTimer *timer;

  timer = win->timer;
  gtk_label_set_text (GTK_LABEL (win->main_timer), "0.0");
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

static void
example_app_window_split_step (ExampleAppWindow *win)
{
  GList *segs;

  segs = win->segments;
  if (segs->next)
  {
    remove_class (GTK_WIDGET (win->segments->data), "current-split");
    win->segments = win->segments->next;
    add_class (GTK_WIDGET (win->segments->data), "current-split");
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
}

static void
format_time (char *buffer, const YurnTime time, const FormatTimeType type)
{
  YurnTime seconds = time;
  char hours, minutes;
  char *buf_cpy;

  buf_cpy = buffer;
  hours = (char) (seconds / (60 * 60));
  seconds -= hours * (60 * 60);
  minutes = (char) (seconds / 60);
  seconds -= minutes * 60;

  hours = abs(hours);
  minutes = abs(minutes);

  switch (type)
  {
    case format_difftime:
      if (time)
      {
        if (time > 0)
          buf_cpy[0] = '+';
        else if (time < 0)
          buf_cpy[0] = '-';
        ++buf_cpy;
      }
    case format_split:
      if (hours)
        sprintf (buf_cpy, "%02u:%02u:%02u", hours, minutes, (uint8_t) seconds);
      else if (minutes)
        sprintf (buf_cpy, "%02u:%02u", minutes, (uint8_t) seconds);
      else
        sprintf (buf_cpy, "%05.2f", seconds);
      break;
    case format_best_times:
      if (hours)
        sprintf (buf_cpy, "%02u:%02u:%05.2f", hours, minutes, seconds);
      else if (minutes)
        sprintf (buf_cpy, "%02u:%05.2f", minutes, seconds);
      else
        sprintf (buf_cpy, "%05.2f", seconds);
      break;
  }
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
  int hours;
  int minutes;
  double seconds;
  ExampleAppWindow *win;

  win = EXAMPLE_APP_WINDOW (data);

  if (win->timer_state != TIMER_STARTED)
    return TRUE;

  seconds = g_timer_elapsed (win->timer, NULL);
  hours = seconds / 3600;
  seconds -= 3600 * hours;
  minutes = seconds / 60;
  seconds -= 60 * minutes;

  if (hours && minutes)
    sprintf (win->current_time, "%02d:%02d:%3.1f", hours, minutes, seconds);
  else if (minutes)
    sprintf (win->current_time, "%02d:%3.1f", minutes, seconds);
  else
    sprintf (win->current_time, "%3.1f", seconds);

  if (strcmp (win->current_time, win->old_time))
  {
    gtk_label_set_text (GTK_LABEL (win->main_timer), win->current_time);
    strcpy (win->old_time, win->current_time);
  }
  return TRUE;
}

static void
example_app_window_init (ExampleAppWindow *win)
{
  GtkCssProvider *win_style;
  GdkScreen *screen;
  GdkDisplay *display;

  win->timer = g_timer_new ();
  g_timer_stop (win->timer);
  win->game = NULL;
  win->timer_state = TIMER_STOPPED;

  gtk_widget_init_template (GTK_WIDGET (win));

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
  GtkWidget *expand_box;
  GtkWidget *split_title;
  GtkWidget *split_time;
  Segment *seg;
  YurnTime sum_of_best_segments;

  game = win->game;
  gtk_label_set_text (GTK_LABEL (win->title), game->title);
  sprintf(buffer, "#%u", game->attempts);
  gtk_label_set_text (GTK_LABEL (win->nr_tries), buffer);
  splits = win->splits;

  for (uint8_t i = 0; i < game->nr_segments; ++i) {
    seg = game->segments[i];
    expand_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_hexpand (expand_box, TRUE);
    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    split_title = gtk_label_new (seg->title);
    split_time = gtk_label_new (NULL);
    if (seg->pb_run)
    {
      format_time (buffer, seg->pb_run, format_split);
      gtk_label_set_text (GTK_LABEL (split_time), buffer);
    } else
    {
      gtk_label_set_text (GTK_LABEL (split_time), "-");
    }
    gtk_container_add (GTK_CONTAINER (hbox), split_title);
    gtk_container_add (GTK_CONTAINER (hbox), expand_box);
    gtk_container_add (GTK_CONTAINER (hbox), split_time);
    gtk_container_add (GTK_CONTAINER (splits), hbox);
  }
  win->segments = gtk_container_get_children (GTK_CONTAINER (win->splits));

  sum_of_best_segments = 0.;
  for (uint8_t i = 0; i < game->nr_segments; ++i)
  {
    sum_of_best_segments += game->segments[i]->best_seg;
  }
  format_time (buffer, sum_of_best_segments, format_best_times);
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
