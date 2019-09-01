#include "gamedata.h"

#include <assert.h>
#include <malloc.h>

Segment *
game_segment_new ()
{
  Segment *seg;
    
  seg = calloc (1, sizeof (Segment));
  return seg;
}

void
game_segment_free (Segment *seg)
{
  free(seg);
}

GameData *
game_data_new ()
{
  GameData *game;
  
  game = calloc (1, sizeof (GameData));
  game->segments = malloc (sizeof(Segment) * 128);
  game->current_run = calloc (1, sizeof (YurnTime));
  game->current_run_idx = 0;
  return game;
}

void
game_data_free (GameData *game)
{
  for (uint8_t i = 0; i < game->nr_segments; ++i)
  {
    game_segment_free(game->segments[i]);
  }
  free (game->segments);
  free (game->current_run);
  free (game);
}

void
game_data_add_segment (GameData *game, Segment *seg)
{
  if (game->nr_segments == 128)
  {
    printf("Error: Maximum number of segments has been reached\n");
    return;
  }
  game->segments[game->nr_segments] = seg;
  if (seg)
    game->nr_segments++;
}

YurnTime
game_data_get_personal_best (GameData *game)
{
  assert (game->nr_segments);

  return game->segments[game->nr_segments - 1]->pb_run;
}

YurnTime
game_data_get_best_segs_sum (GameData *game)
{
  int i;

  if (game->sum_of_best_segs)
    return game->sum_of_best_segs;

  for (i = 0; i < game->nr_segments; ++i)
    game->sum_of_best_segs += game->segments[i]->best_seg;

  return game->sum_of_best_segs;
}
