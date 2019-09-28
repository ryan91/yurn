#ifndef __GAME_DATA_H
#define __GAME_DATA_H

#include <stdbool.h>
#include <stdint.h>

#define YURN_TIME_INVALID -1.

typedef double YurnTime;
struct GameData;

enum TimeClassification
{
  TIME_CLASS_BETTER_THAN_BEST_SEG,
  TIME_CLASS_BETTER_THAN_PB,
  TIME_CLASS_WORSE_THAN_PB,
  TIME_CLASS_LOSING,
  TIME_CLASS_INVALID,
};

struct GameData        *game_data_new                       ();
void                    game_data_copy                      (struct GameData *destination,
                                                             const struct GameData *source);
void                    game_data_free                      (struct GameData *game);
YurnTime                game_data_get_current_pb            (const struct GameData *game);
YurnTime                game_data_get_pb                    (const struct GameData *game);
YurnTime                game_data_get_current_best_seg      (const struct GameData *game);
const char             *game_data_get_seg_title             (const struct GameData *game,
                                                             const uint8_t seg_i);
YurnTime                game_data_get_seg_pb                (const struct GameData *game,
                                                             const uint8_t seg_i);
YurnTime                game_data_get_seg_best              (const struct GameData *game,
                                                             const uint8_t seg_i);
YurnTime                game_data_get_pb_run_i              (const struct GameData *game,
                                                             const uint8_t seg_i);
const char             *game_data_get_game_title            (const struct GameData *game);
uint8_t                 game_data_get_nr_segments           (const struct GameData *game);
YurnTime                game_data_get_sum_of_best_segs      (const struct GameData *game);
YurnTime                game_data_get_bests_from_now_on     (const struct GameData *game);
YurnTime                game_data_get_segment_time          (const struct GameData *game);
uint16_t                game_data_get_attempts              (const struct GameData *game);
enum TimeClassification game_data_get_classification        (const struct GameData *game);
void                    game_data_add_segment               (struct GameData *game,
                                                             const YurnTime bp,
                                                             const YurnTime best_seg,
                                                             const char *title);
void                    game_data_set_game_title            (struct GameData *game,
                                                             const char *title);
void                    game_data_set_attempts              (struct GameData *game,
                                                             const uint16_t attempts);
void                    game_data_update_current_segment    (struct GameData *game,
                                                             const YurnTime total_elapsed,
                                                             const YurnTime seg_start);
void                    game_data_advance_segment           (struct GameData *game,
                                                             const YurnTime time);
bool                    game_data_set_current_time          (struct GameData *game,
                                                             const YurnTime current_time);

#endif
