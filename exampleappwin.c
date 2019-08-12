#include <gtk/gtk.h>

#include "yurn_style.h"

#include "exampleapp.h"
#include "exampleappwin.h"
#include "jsonparser.h"

static void
format_time (char *buffer, const YurnTime *time)
{
  if (time->hours)
  {
    if (time->miliseconds)
    {
      sprintf (buffer, "%02u:%02u:%02u.%02u", time->hours, time->minutes,
               time->seconds, time->miliseconds);
    } else
    {
      sprintf (buffer, "%02u:%02u:%02u", time->hours, time->minutes,
               time->seconds);
    }
  } else
  {
    sprintf (buffer, "%02u:%02u", time->minutes, time->seconds);
  }
}

struct _ExampleAppWindow
{
  GtkApplicationWindow parent;

  GtkWidget *title;
  GtkWidget *nr_tries;
  GtkWidget *timer_seconds;
  GtkWidget *timer_decimal;
  GtkWidget *previous_segment;
  GtkWidget *best_possible_time;
  GtkWidget *personal_best;
  GtkWidget *splits;

  GameData *game;
};

G_DEFINE_TYPE (ExampleAppWindow, example_app_window, GTK_TYPE_APPLICATION_WINDOW)

static void
example_app_window_init (ExampleAppWindow *win)
{
  GtkCssProvider *win_style;
  GdkScreen *screen;
  GdkDisplay *display;

  win->game = NULL;

  gtk_widget_init_template (GTK_WIDGET (win));

  win_style = gtk_css_provider_new ();
  display = gdk_display_get_default ();
  screen = gdk_display_get_default_screen (display);
  gtk_style_context_add_provider_for_screen (
      screen,
      GTK_STYLE_PROVIDER(win_style),
      GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  gtk_css_provider_load_from_data (GTK_CSS_PROVIDER (win_style),
                                   (char *) yurn_css, yurn_css_len, NULL);

  gtk_style_context_add_class (
      gtk_widget_get_style_context (win->timer_seconds),
      "timer-seconds");
  gtk_style_context_add_class (
      gtk_widget_get_style_context (win->timer_decimal),
      "timer-decimal");
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
                                        ExampleAppWindow, timer_seconds);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class),
                                        ExampleAppWindow, timer_decimal);
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
example_app_window_load_game (ExampleAppWindow *win)
{
  GameData *game;
  char buffer[16];
  GtkWidget *splits;
  GtkWidget *hbox;
  GtkWidget *expand_box;
  GtkWidget *split_title;
  GtkWidget *split_time;
  Segment *seg;
  YurnTime *time;
  YurnTime *pb;
  char best_sum_valid;

  game = win->game;
  gtk_label_set_text (GTK_LABEL (win->title), game->title);
  sprintf(buffer, "#%u", game->attempts);
  gtk_label_set_text (GTK_LABEL (win->nr_tries), buffer);
  splits = win->splits;
  pb = calloc (1, sizeof (YurnTime));
  best_sum_valid = 1;

  for (uint8_t i = 0; i < game->nr_segments; ++i) {
    seg = game->segments[i];
    expand_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_hexpand (expand_box, TRUE);
    time = seg->time;
    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    split_title = gtk_label_new (seg->title);
    split_time = gtk_label_new (NULL);
    if (time)
    {
      if (best_sum_valid)
      {
        add_times_inplace (pb, time);
      }
      format_time (buffer, time);
      gtk_label_set_text (GTK_LABEL (split_time), buffer);
    } else
    {
      gtk_label_set_text (GTK_LABEL (split_time), "-");
      best_sum_valid = 0;
    }
    gtk_container_add (GTK_CONTAINER (hbox), split_title);
    gtk_container_add (GTK_CONTAINER (hbox), expand_box);
    gtk_container_add (GTK_CONTAINER (hbox), split_time);
    gtk_container_add (GTK_CONTAINER (splits), hbox);
  }

  if (best_sum_valid)
  {
    format_time (buffer, pb);
    gtk_label_set_text (GTK_LABEL (win->personal_best), buffer);
  }

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
    example_app_window_load_game (win);
  }
}
