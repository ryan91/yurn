#include "gamedata.h"

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
  
  game = calloc(1, sizeof (GameData));
  game->segments = malloc(sizeof(Segment) * 128);
  return game;
}

void
game_data_free (GameData *game)
{
  for (uint8_t i = 0; i < game->nr_segments; ++i)
  {
    game_segment_free(game->segments[i]);
  }
  free(game->segments);
  free(game);
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
