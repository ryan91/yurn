#include <gtk/gtk.h>

#include "yurn_style.h"

#include "exampleapp.h"
#include "exampleappwin.h"

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
};

G_DEFINE_TYPE (ExampleAppWindow, example_app_window, GTK_TYPE_APPLICATION_WINDOW)

static void
example_app_window_init (ExampleAppWindow *win)
{
  GtkCssProvider *win_style;
  GdkScreen *screen;
  GdkDisplay *display;

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
}

ExampleAppWindow *
example_app_window_new (ExampleApp *app)
{
  return g_object_new (EXAMPLE_APP_WINDOW_TYPE, "application", app, NULL);
}

void
example_app_window_open (ExampleAppWindow *win,
                         GFile            *file)
{
  //gchar *basename;
  //GtkWidget *scrolled, *view;
  //gchar *contents;
  //gsize length;

  //basename = g_file_get_basename (file);

  //scrolled = gtk_scrolled_window_new (NULL, NULL);
  //gtk_widget_set_hexpand (scrolled, TRUE);
  //gtk_widget_set_vexpand (scrolled, TRUE);
  //view = gtk_text_view_new ();
  //gtk_text_view_set_editable (GTK_TEXT_VIEW (view), FALSE);
  //gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (view), FALSE);
  //gtk_container_add (GTK_CONTAINER (scrolled), view);
  //// gtk_stack_add_titled (GTK_STACK (win->stack), scrolled, basename, basename);

  //if (g_file_load_contents (file, NULL, &contents, &length, NULL, NULL))
  //  {
  //    GtkTextBuffer *buffer;

  //    buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
  //    gtk_text_buffer_set_text (buffer, contents, length);
  //    g_free (contents);
  //  }

  //g_free (basename);
}
