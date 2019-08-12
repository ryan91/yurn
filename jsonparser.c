#include "jsonparser.h"

#include <jansson.h>
#include <string.h>

static inline char
is_number (const char *c)
{
  for (uint8_t i = 0; c[i] != 0; ++i) {
    if (! ('0' <= c[i] && c[i] <= '9'))
    {
      return 0;
    }
  }
  return 1;
}

static YurnTime *
parse_json_time (const char *time_str)
{
  YurnTime *time;
  char str[3];

  time = malloc (sizeof (YurnTime));
  str[2] = 0;

  str[0] = time_str[0];
  str[1] = time_str[1];
  if (! is_number (str))
  {
    goto fail;
  }
  time->hours = atoi (str);

  str[0] = time_str[3];
  str[1] = time_str[4];
  if (! is_number (str))
  {
    goto fail;
  }
  time->minutes = atoi (str);

  str[0] = time_str[6];
  str[1] = time_str[7];
  if (! is_number (str))
  {
    goto fail;
  }
  time->seconds = atoi (str);

  str[0] = time_str[9];
  str[1] = time_str[10];
  if (! is_number (str))
  {
    goto fail;
  }
  time->miliseconds = atoi (str);
 
  return time;

fail:
  free (time);
  return NULL;
}

GameData *
json_parser_read_file (const char *filename)
{
  json_t *root;
  json_t *json_segment;
  json_t *ref;
  json_t *splits;
  YurnTime *time;
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
        strcpy (seg->title, json_string_value (ref));
      }
      ref = json_object_get (json_segment, "best");
      if (ref)
      {
        time = parse_json_time (json_string_value (ref));
        seg->time = time;
      }
      game_data_add_segment (game, seg);
    }
  }

  json_segment = json_object_get (root, "world_record");
  if (json_segment)
  {
    ref = json_object_get (json_segment, "time");
    if (ref)
    {
      time = parse_json_time (json_string_value (ref));
      game->wr_time = time;
    }
    ref = json_object_get (json_segment, "by");
    if (ref)
    {
      strcpy (game->wr_by, json_string_value (ref));
    }
  }

  return game;
}
