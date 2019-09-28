#include "gamedata.h"

#include <assert.h>
#include <malloc.h>
#include <string.h>

struct GameData
{
  char title[128];
  struct Segment
  {
    YurnTime pb;
    YurnTime best_seg;
    char title[64];
  } segments[128];
  uint8_t nr_segments;
  uint8_t current_segment;
  uint16_t attempts;
  YurnTime run[128];
  YurnTime sum_of_best_segments;
  enum TimeClassification tc;
};

struct GameData *
game_data_new ()
{
  struct GameData *game = calloc (1, sizeof (struct GameData));
  game->sum_of_best_segments = YURN_TIME_INVALID;
  return game;
}

void
game_data_copy (struct GameData *destination,
                 const struct GameData *source)
{
  memcpy (destination, source, sizeof (struct GameData));
}

void
game_data_free (struct GameData *game)
{
  free (game);
}

YurnTime
game_data_get_current_pb (const struct GameData *game)
{
  assert (game->current_segment < game->nr_segments);
  return game->segments[game->current_segment].pb;
}

YurnTime
game_data_get_pb (const struct GameData *game)
{
  assert (game->nr_segments);
  return game->segments[game->nr_segments - 1].pb;
}

YurnTime
game_data_get_current_best_seg (const struct GameData *game)
{
  assert (game->current_segment < game->nr_segments);
  return game->segments[game->current_segment].best_seg;
}

const char
*game_data_get_seg_title (const struct GameData *game, const uint8_t seg_i)
{
  assert (seg_i < game->nr_segments);
  return game->segments[seg_i].title;
}

YurnTime
game_data_get_seg_pb (const struct GameData *game, const uint8_t seg_i)
{
  assert (seg_i < game->nr_segments);
  return game->segments[seg_i].pb;
}

YurnTime
game_data_get_seg_best (const struct GameData *game, const uint8_t seg_i)
{
  assert (seg_i < game->nr_segments);
  return game->segments[seg_i].best_seg;
}

const char *
game_data_get_game_title (const struct GameData *game)
{
  return game->title;
}

uint8_t
game_data_get_nr_segments (const struct GameData *game)
{
  return game->nr_segments;
}

YurnTime
game_data_get_sum_of_best_segs (const struct GameData *game)
{
  return game->sum_of_best_segments;
}

YurnTime game_data_get_bests_from_now_on (const struct GameData *game)
{
  uint8_t i;
  YurnTime sum_from_now_on = 0.;
  for (i = game->current_segment + 1; i < game->nr_segments; ++i)
  {
    sum_from_now_on += game->segments[i].best_seg;
  }
  return sum_from_now_on;
}

YurnTime
game_data_get_segment_time (const struct GameData *game)
{
  return game->run[game->current_segment];
}

uint16_t
game_data_get_attempts (const struct GameData *game)
{
  return game->attempts;
}

enum TimeClassification
game_data_get_classification (const struct GameData *game)
{
  return game->tc;
}

void
game_data_add_segment (struct GameData *game, const YurnTime pb,
                       const YurnTime best_seg, const char *title)
{
  struct Segment *new_seg;

  assert (game->nr_segments < 128);
  new_seg = &game->segments[game->nr_segments];
  new_seg->pb = pb;
  new_seg->best_seg = best_seg;
  strcpy (new_seg->title, title);
  ++game->nr_segments;
  if (game->sum_of_best_segments != YURN_TIME_INVALID)
    game->sum_of_best_segments += best_seg;
}

void
game_data_set_game_title  (struct GameData *game, const char *title)
{
  strcpy (game->title, title);
}

void
game_data_set_attempts (struct GameData *game, const uint16_t attempts)
{
  game->attempts = attempts;
}


void
game_data_update_current_segment (struct GameData *game,
                                  const YurnTime total_elapsed,
                                  const YurnTime seg_start)
{
  struct Segment *seg;
  YurnTime seg_time;

  seg = &game->segments[game->current_segment];
  if (total_elapsed < seg->pb)
    seg->pb = total_elapsed;
  seg_time = total_elapsed - seg_start;
  if (seg_time < seg->best_seg)
    seg->best_seg = seg_time;
}

void
game_data_advance_segment (struct GameData *game, const YurnTime time)
{
  assert (game->current_segment < game->nr_segments);

  YurnTime *run = game->run;
  uint8_t cur_seg = game->current_segment;
  struct Segment *seg = &game->segments[cur_seg];
  YurnTime time_in_previous_segment;

  run[cur_seg] = time;
  time_in_previous_segment = cur_seg ? time - run[cur_seg - 1] : time;
  if (seg->best_seg == YURN_TIME_INVALID ||
      time_in_previous_segment < seg->best_seg)
    seg->best_seg = time_in_previous_segment;
  ++game->current_segment;
}

bool
game_data_set_current_time (struct GameData *game, const YurnTime current_time)
{
  enum TimeClassification old_class = game->tc;
  uint8_t cur_seg = game->current_segment;
  YurnTime *run = game->run;
  struct Segment *seg = &game->segments[cur_seg];

  run[cur_seg] = cur_seg ? current_time - run[cur_seg - 1] : current_time;

  if (seg->best_seg == YURN_TIME_INVALID || seg->pb == YURN_TIME_INVALID)
    game->tc = TIME_CLASS_INVALID;
  else if (run[cur_seg] < seg->best_seg)
    game->tc = TIME_CLASS_BETTER_THAN_BEST_SEG;
  else if (current_time < seg->pb)
    game->tc = TIME_CLASS_BETTER_THAN_PB;
  else if (current_time + game_data_get_bests_from_now_on (game) >
           game_data_get_pb (game))
    game->tc = TIME_CLASS_LOSING;
  else
    game->tc = TIME_CLASS_WORSE_THAN_PB;

  return old_class != game->tc;
}

YurnTime
game_data_get_pb_run_i (const struct GameData *game,
                        const uint8_t seg_i)
{
  assert (game->current_segment == game->nr_segments);
  assert (seg_i < game->nr_segments);
  uint8_t last_seg = game->nr_segments - 1;

  if (game->segments[seg_i].pb == YURN_TIME_INVALID)
    return game->run[seg_i];

  return game->segments[last_seg].pb < game->run[last_seg] ?
    game->segments[seg_i].pb : game->run[seg_i];
}
