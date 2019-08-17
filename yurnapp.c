#include <gtk/gtk.h>
#include <assert.h>

#include "yurnapp.h"
#include "yurnappwin.h"

struct _YurnApp
{
  GtkApplication parent;
};

G_DEFINE_TYPE(YurnApp, yurn_app, GTK_TYPE_APPLICATION);

static void
yurn_app_init (YurnApp *app)
{
}

static void
quit_activated (GSimpleAction *action,
                GVariant      *parameter,
                gpointer       app)
{
  g_application_quit (G_APPLICATION (app));
}

static void
app_menu_open (GSimpleAction *action,
               GVariant      *parameter,
               gpointer      app)
{
  GList                *windows;
  GtkWidget            *dialog;
  YurnAppWin           *win;
  gint                 res;
  char                 *filename;

  windows = gtk_application_get_windows (GTK_APPLICATION (app));
  win = YURN_APP_WIN (windows->data);
  assert (windows && "Windows should not be NULL here");
  dialog = gtk_file_chooser_dialog_new ("Open File", GTK_WINDOW (win),
                                        GTK_FILE_CHOOSER_ACTION_OPEN,
                                        "_Cancel", GTK_RESPONSE_CANCEL,
                                        "_Open", GTK_RESPONSE_ACCEPT, NULL);
  res = gtk_dialog_run (GTK_DIALOG (dialog));
  if (res == GTK_RESPONSE_ACCEPT)
  {
    filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
    yurn_app_win_open (win, filename);
    g_free (filename);
  }
  gtk_widget_destroy (dialog);
}

static void
app_menu_save (GSimpleAction *action,
               GVariant      *parameter,
               gpointer      app)
{
  printf("TODO: save\n");
}

static void
app_menu_reload (GSimpleAction *action, 
                 GVariant      *parameter,
                 gpointer      app)
{
  printf("TOOD: reload\n");
}

static GActionEntry app_entries[] =
{
  { "open",        app_menu_open,         NULL, NULL, NULL },
  { "save",        app_menu_save,         NULL, NULL, NULL },
  { "reload",      app_menu_reload,       NULL, NULL, NULL },
  { "quit",        quit_activated,        NULL, NULL, NULL }
};

static void
yurn_app_startup (GApplication *app)
{
  GtkBuilder           *builder;
  GMenuModel           *app_menu;
  const gchar          *open_accels[2] = { "<Ctrl>O", NULL };
  const gchar          *save_accels[2] = { "<Ctrl>S", NULL };
  const gchar          *quit_accels[2] = { "<Ctrl>Q", NULL };

  G_APPLICATION_CLASS (yurn_app_parent_class)->startup (app);

  g_action_map_add_action_entries (G_ACTION_MAP (app),
                                   app_entries, G_N_ELEMENTS (app_entries),
                                   app);
  gtk_application_set_accels_for_action (GTK_APPLICATION (app),
                                         "app.open",
                                         open_accels
                                         );
  gtk_application_set_accels_for_action (GTK_APPLICATION (app),
                                         "app.save",
                                         save_accels
                                         );
  gtk_application_set_accels_for_action (GTK_APPLICATION (app),
                                         "app.quit",
                                         quit_accels
                                         );

  builder = gtk_builder_new_from_resource ("/org/gtk/yurn/app-menu.ui");
  app_menu = G_MENU_MODEL (gtk_builder_get_object (builder, "appmenu"));
  gtk_application_set_app_menu (GTK_APPLICATION (app), app_menu);
  g_object_unref (builder);
}

static void
yurn_app_activate (GApplication *app)
{
  YurnAppWin           *win;

  win = yurn_app_win_new (YURN_APP (app));
  gtk_window_present (GTK_WINDOW (win));
}

static void
yurn_app_open (GApplication  *app,
               GFile        **files,
               gint           n_files,
               const gchar   *hint)
{
  GList                *windows;
  YurnAppWin           *win;

  windows = gtk_application_get_windows (GTK_APPLICATION (app));
  if (windows)
    win = YURN_APP_WIN (windows->data);
  else
    win = yurn_app_win_new (YURN_APP (app));

  gtk_window_present (GTK_WINDOW (win));
}

static void
yurn_app_class_init (YurnAppClass *class)
{
  G_APPLICATION_CLASS (class)->startup = yurn_app_startup;
  G_APPLICATION_CLASS (class)->activate = yurn_app_activate;
  G_APPLICATION_CLASS (class)->open = yurn_app_open;
}

YurnApp *
yurn_app_new (void)
{
  return g_object_new (YURN_APP_TYPE,
                       "application-id", "org.gtk.yurn",
                       "flags", G_APPLICATION_HANDLES_OPEN,
                       NULL);
}
