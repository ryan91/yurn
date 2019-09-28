#include "jsonparser.h"

#include <jansson.h>
#include <string.h>

static inline char
is_number (const char *c)
{
  for (uint8_t i = 0; c[i] != 0; ++i)
    if (! ('0' <= c[i] && c[i] <= '9'))
      return 0;
  return 1;
}

struct GameData *
json_parser_read_file (const char *filename)
{
  json_t                *root;
  json_t                *json_segment;
  json_t                *ref;
  json_t                *splits;
  size_t                nr_splits;
  size_t                i;
  json_error_t          error;
  struct GameData      *game;
  char                  buffer[128];
  YurnTime              best_seg;
  YurnTime              pb;

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
    strcpy (buffer, json_string_value (ref));
    game_data_set_game_title (game, buffer);
  }
  else
    game_data_set_game_title (game, "No Title Given");

  ref = json_object_get (root, "attempts");
  if (ref)
    game_data_set_attempts (game, json_integer_value (ref));

//  ref = json_object_get (root, "start_delay");
//  if (ref)
//    game->start_delay = (float) json_real_value (ref);

  splits = json_object_get (root, "splits");
  if (splits)
  {
    nr_splits = json_array_size (splits);
    for (i = 0; i < nr_splits; ++i)
    {
      json_segment = json_array_get (splits, i);
      ref = json_object_get (json_segment, "title");
      if (ref)
        strcpy (buffer, json_string_value (ref));
      else
        strcpy (buffer, "<No segment title>");
      ref = json_object_get (json_segment, "best seg");
      if (ref)
        best_seg = json_real_value (ref);
      else
        best_seg = YURN_TIME_INVALID;
      ref = json_object_get (json_segment, "pb run");
      if (ref)
        pb = json_real_value (ref);
      else
        pb = YURN_TIME_INVALID;
      game_data_add_segment (game, pb, best_seg, buffer);
    }
  }

  json_decref (root);

//  json_segment = json_object_get (root, "world_record");
//  if (json_segment)
//  {
//    ref = json_object_get (json_segment, "time");
//    if (ref)
//      game->wr_time = json_real_value (ref);
//    ref = json_object_get (json_segment, "by");
//    if (ref)
//      strcpy (game->wr_by, json_string_value (ref));
//  }

  return game;
}

bool
json_parser_write_file (const struct GameData *game,
                        const char *filename)
{
  json_t *root;
  json_t *splits;
  json_t *split;

  root = json_object ();

  json_object_set_new (root, "title",
                       json_string (game_data_get_game_title (game)));
  json_object_set_new (root, "attempts",
                       json_integer (game_data_get_attempts (game)));

  splits = json_array ();
  for (uint8_t i = 0; i < game_data_get_nr_segments (game); ++i)
  {
    split = json_object ();
    json_object_set (split, "title",
                     json_string (game_data_get_seg_title (game, i)));
    json_object_set (split, "pb run",
                     json_real (game_data_get_pb_run_i (game, i)));
    json_object_set (split, "best seg",
                     json_real (game_data_get_seg_best (game, i)));
    json_array_append (splits, split);
    json_decref (split);
  }
  json_object_set (root, "splits", splits);

  bool ret = !json_dump_file (root, filename, 0);

  json_decref (splits);
  json_decref (root);

  return ret;
}
