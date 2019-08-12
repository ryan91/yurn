#include <gtk/gtk.h>

#include "yurn_style.h"

#include "exampleapp.h"
#include "exampleappwin.h"
#include "jsonparser.h"

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

typedef enum _FormatTimeType
{
  format_split,
  format_difftime, 
  format_best_times
} FormatTimeType;

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
    example_app_window_load_game (win);
  }
}
