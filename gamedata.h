#ifndef __GAME_DATA_H
#define __GAME_DATA_H

#include <stdint.h>

typedef double YurnTime;

typedef struct _Segment
{
  char                  title[32];
  YurnTime              best_seg;
  YurnTime              pb_run;

} Segment;

typedef struct _GameData
{
  char                  title[128];
  uint32_t              attempts;
  float                 start_delay;
  Segment             **segments;
  uint8_t               nr_segments;
  YurnTime              wr_time;
  char                  wr_by[32];

} GameData;

Segment                *game_segment_new                ();
void                    game_segment_free               (Segment *seg);
GameData               *game_data_new                   ();
void                    game_data_free                  (GameData *game);
void                    game_data_add_segment           (GameData *game, Segment *seg);

#endif
