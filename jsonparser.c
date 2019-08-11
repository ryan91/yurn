#include "jsonparser.h"

#include <jansson.h>
#include <string.h>

GameData *
json_parser_read_file (const char *filename)
{
  json_t *root;
  json_t *json_segment;
  json_t *ref;
  json_t *splits;
  size_t nr_splits;
  json_error_t error;
  GameData *game;
  Segment *seg;

  root = json_load_file (filename, 0, &error);

  game = game_data_new ();

  if (!root)
  {
    fprintf (stderr, "Error reading JSON line %d: %s\n", error.line, error.text);
    return NULL;
  }

  ref = json_object_get (root, "title");
  if (ref)
  {
    strcpy (game->title, json_string_value (ref));
  } else
  {
    strcpy (game->title, "No Title Given");
  }

  ref = json_object_get (root, "attempts");
  if (ref)
  {
    game->attempts = json_integer_value (ref);
  }

  ref = json_object_get (root, "start_delay");
  if (ref)
  {
    game->start_delay = (float) json_real_value (ref);
  }

  splits = json_object_get (root, "splits");
  if (splits)
  {
    nr_splits = json_array_size (splits);
    for (size_t i = 0; i < nr_splits; ++i)
    {
      json_segment = json_array_get (splits, i);
      seg = game_segment_new ();
      ref = json_object_get (json_segment, "title");
      if (ref)
      {
        strcpy(seg->title, json_string_value (ref));
      }
      // TODO time
      game_data_add_segment (game, seg);
    }
  }

  return game;
}
