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
  YurnTime              sum_of_best_segs;
  YurnTime              wr_time;
  YurnTime             *current_run;
  int                   current_run_idx;
  char                  wr_by[32];
} GameData;

Segment                *game_segment_new                ();
void                    game_segment_free               (Segment *seg);
GameData               *game_data_new                   ();
void                    game_data_free                  (GameData *game);
void                    game_data_add_segment           (GameData *game, Segment *seg);
YurnTime                game_data_get_personal_best     (GameData *game);
YurnTime                game_data_get_best_segs_sum     (GameData *game);

#endif
