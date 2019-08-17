#include <gtk/gtk.h>

#include "yurnapp.h"

int
main (int argc, char *argv[])
{
  return g_application_run (G_APPLICATION (yurn_app_new ()), argc, argv);
}
