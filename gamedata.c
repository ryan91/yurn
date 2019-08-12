#include "gamedata.h"

#include <malloc.h>

Segment *
game_segment_new ()
{
  Segment *seg;
    
  seg = malloc(sizeof(Segment));
  seg->time = NULL;
  return seg;
}

void
game_segment_free (Segment *seg)
{
  free(seg->time);
  free(seg);
}

GameData *
game_data_new ()
{
  GameData *game;
  
  game = malloc(sizeof(GameData));
  game->segments = malloc(sizeof(Segment) * 128);
  game->nr_segments = 0;
  game->attempts = 0;
  game->start_delay = 0.f;
  game->title[0] = 0;
  game->wr_time = NULL;
  game->wr_by[0] = 0;
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
  free(game->wr_time);
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
  game->segments[game->nr_segments++] = seg;
}

static void
__add_times(YurnTime *res, const YurnTime *t1, const YurnTime *t2)
{
  uint8_t carry;
  uint32_t sum;

  sum = t1->miliseconds + t2->miliseconds;
  res->miliseconds = sum % 100;
  carry = sum / 100;

  sum = t1->seconds + t2->seconds + carry;
  res->seconds = sum % 60;
  carry = sum / 60;

  sum = t1->minutes + t2->minutes + carry;
  res->minutes = sum % 60;
  carry = sum / 60;

  res->hours = t1->hours + t2->hours + carry;
}

YurnTime *
add_times (const YurnTime *t1, const YurnTime *t2)
{
  YurnTime *res;

  res = malloc (sizeof (YurnTime));
  __add_times (res, t1, t2);
  return res;
}

void
add_times_inplace (YurnTime *t1, const YurnTime *t2)
{
  __add_times (t1, t1, t2);
}
