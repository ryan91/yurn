#include "yurn_state_machine.h"

static TransFunc transitions[YURN_NR_OF_STATES][YURN_NR_OF_INPUTS];

static YurnState current_state;

void
yurn_sm_set_transition (YurnState initial_state,
                        YurnInput input,
                        YurnState (*func) (gpointer data))
{
  transitions[initial_state][input] = func;
}

YurnState
yurn_sm_transition (YurnInput input, gpointer data)
{
  TransFunc func = transitions[current_state][input];
  if (func)
    return func(data);
  return current_state;
}

YurnState
yurn_sm_get_current_state ()
{
  return current_state;
}

void
yurn_sm_set_global_transition (YurnInput input, TransFunc f)
{
  int i;

  for (i = 0; i < YURN_NR_OF_STATES - 1; ++i)
    transitions[i][input] = f;
}
