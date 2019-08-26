#ifndef __YURN_STATE_MACHINE_H
#define __YURN_STATE_MACHINE_H

#include <gtk/gtk.h>

#define YURN_NR_OF_STATES 5
#define YURN_NR_OF_INPUTS 7

typedef enum _YurnState
{
  YURN_STATE_INITIAL,
  YURN_STATE_GAME_LOADED,
  YURN_STATE_TIMER_RUNNING,
  YURN_STATE_TIMER_PAUSED,
  YURN_STATE_RUN_FINISHED,
} YurnState;

typedef YurnState (*TransFunc) (gpointer data);

typedef struct _TransitionData
{
  YurnState new_state;
  
} TransitionData;

typedef enum _YurnInput
{
  YURN_INPUT_SPACE,
  YURN_INPUT_F5,
  YURN_INPUT_F3,
  YURN_INPUT_OPEN,
  YURN_INPUT_SAVE,
  YURN_INPUT_RELOAD,
  YURN_INPUT_QUIT
} YurnInput;

void                    yurn_sm_set_transition          (YurnState initial_state,
                                                         YurnInput input,
                                                         TransFunc f);
void                    yurn_sm_set_global_transition   (YurnInput input,
                                                         TransFunc f);
YurnState               yurn_sm_transition              (YurnInput input,
                                                         gpointer data);
YurnState               yurn_sm_get_current_state       ();

#endif
