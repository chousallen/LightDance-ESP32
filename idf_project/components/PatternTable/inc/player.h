#pragma once

#ifdef __cplusplus
extern "C" {
#endif


#include "player_content.h"
#include "sdmmc_cmd.h"


//init player, use it before start the player
void player_init(player *p);

// start player playing
void player_start(player *p);

// resume player
void player_resume(player *p);

//pause player
void player_pause(player *p);

//stop player
void player_stop(player *p);

//delaytime for player (reserved stage, content not finished)
void player_delay(player *p,int delaytime,int delaylight);

//exit player(which means program end)
void player_exit(player *p);

// log out current stage and return
PlayerState player_get_state(player *p);

#ifdef __cplusplus
}
#endif
