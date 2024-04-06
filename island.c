/*
 *  This file is part of 'Johnny Reborn'
 *
 *  An open-source engine for the classic
 *  'Johnny Castaway' screensaver by Sierra.
 *
 *  Copyright (C) 2019 Jeremie GUILLAUME
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include <stdlib.h>
#include <stdio.h>

#include "mytypes.h"
#include "graphics.h"
#include "island.h"
#include "utils.h"


struct TIslandState islandState = { 0, 0, 0, 0, 0, 0, 0, {0,0,0,0,0}, {0,0,0,0,0} };


void islandInit(struct TTtmThread *ttmThread)
{
    struct TTtmSlot *ttmSlot = ttmThread->ttmSlot;


    if (islandState.night) {
        grLoadScreen("NIGHT.SCR");
    }
    else {
        char scrName[12];
        sprintf(scrName, "OCEAN0%d.SCR", rand() % 3);
        grLoadScreen(scrName);
    }

    ttmThread->ttmLayer = grBackgroundSfc;

    grDx = islandState.xPos;
    grDy = islandState.yPos;


    // Raft

    grLoadBmp(ttmSlot, 0, "MRAFT.BMP");

    sint32 xRaft = (islandState.lowTide ? 529 : 512);
    sint32 yRaft = (islandState.lowTide ? 281 : 266);

    switch (islandState.raft) {
        case 1: grDrawSprite(grBackgroundSfc, ttmSlot, xRaft, yRaft, 0, 0); break;  // raft-1
        case 2: grDrawSprite(grBackgroundSfc, ttmSlot, xRaft, yRaft, 1, 0); break;  // raft-2
        case 3: grDrawSprite(grBackgroundSfc, ttmSlot, xRaft, yRaft, 2, 0); break;  // raft-3
        case 4: grDrawSprite(grBackgroundSfc, ttmSlot, xRaft, yRaft, 3, 0); break;  // raft-4
        case 5: grDrawSprite(grBackgroundSfc, ttmSlot, xRaft, yRaft, 4, 0); break;  // raft-5
    }


    grLoadBmp(ttmSlot, 0, "BACKGRND.BMP");


    // Clouds
    
    grDx = grDy = 0;

    uint16 cloudX, cloudY;

    sint32 numClouds = rand() % 6;
    sint32 windDirection = rand() % 2;

    islandState.clouds.numClouds = numClouds;
    islandState.clouds.windDirection = windDirection;

    for (sint32 i=0; i < numClouds; i++) {
        sint32 cloudNo = rand() % 3;
        switch (cloudNo) {
            case 0:
                cloudX = rand() % (SCREEN_WIDTH - 129);
                cloudY = rand() % (100 - 36 ) + 25;
                break;

            case 1:
                cloudX = rand() % (SCREEN_WIDTH - 192);
                cloudY = rand() % (100 - 57 ) + 25;
                break;

            case 2:
                cloudX = rand() % (SCREEN_WIDTH - 264);
                cloudY = rand() % (100 - 76 ) + 25;
                break;
        }
        islandState.clouds.windSpeed[i] = rand() % 2 + 1;
        islandState.clouds.cloudNo[i] = cloudNo;
        islandState.clouds.xPos[i] = cloudX;
        islandState.clouds.yPos[i] = cloudY;
    }

    grDx = islandState.xPos;
    grDy = islandState.yPos;

    // The island itself

    grDrawSprite(grBackgroundSfc, ttmSlot, 288, 279,  0, 0);      // island
    grDrawSprite(grBackgroundSfc, ttmSlot, 442, 148, 13, 0);      // trunk
    grDrawSprite(grBackgroundSfc, ttmSlot, 365, 122, 12, 0);      // leafs
    grDrawSprite(grBackgroundSfc, ttmSlot, 396, 279, 14, 0);      // palmtree's shadow

    if (islandState.lowTide) {
        grDrawSprite(grBackgroundSfc, ttmSlot, 249, 303,  1, 0);  // low tide shore
        grDrawSprite(grBackgroundSfc, ttmSlot, 150, 328,  2, 0);  // rock
    }

    // Initial waves on the shore
    for (int i=0; i < 4; i++) {
        islandAnimate(ttmThread);
    }

    // Waves animation thread
    ttmThread->delay = ttmThread->timer = 8;
}

void islandAnimate(struct TTtmThread *ttmThread)
{
    static sint32 counter1 = 0;
    static sint32 counter2 = 0;

    struct TTtmSlot *ttmSlot = ttmThread->ttmSlot;

    grDx = islandState.xPos;
    grDy = islandState.yPos;

    counter2++;
    if (islandState.lowTide) {
        counter2 %= 4;
        switch (counter2) {
            case 0: grDrawSprite(grBackgroundSfc, ttmSlot, 129, 340, 39+counter1, 0); break;  // rock waves (40)
            case 1: grDrawSprite(grBackgroundSfc, ttmSlot, 233, 323, 30+counter1, 0); break;  // low tide waves - left (31)
            case 2: grDrawSprite(grBackgroundSfc, ttmSlot, 367, 356, 33+counter1, 0); break;  // low tide waves - center (33)
            case 3: grDrawSprite(grBackgroundSfc, ttmSlot, 558, 323, 36+counter1, 0); break;  // low tide waves - right (36)
        }
    } else {
        counter2 %= 3;
        switch (counter2) {
            case 0: grDrawSprite(grBackgroundSfc, ttmSlot, 270, 306, 3+counter1, 0); break;  // high tide waves - left (3)
            case 1: grDrawSprite(grBackgroundSfc, ttmSlot, 364, 319, 6+counter1, 0); break;  // high tide waves - center (6)
            case 2: grDrawSprite(grBackgroundSfc, ttmSlot, 518, 303, 9+counter1, 0); break;  // high tide waves - right (9)
        }
    }

    if (!counter2) {
        counter1++;
        counter1 %= 3;
    }
}

void islandInitHoliday(struct TTtmThread *ttmThread) {
    struct TTtmSlot *ttmSlot = ttmThread->ttmSlot;

    if (islandState.holiday) {
        ttmThread->ttmLayer  = grNewLayer();
        ttmThread->isRunning = 3;

        grDx = islandState.xPos;
        grDy = islandState.yPos;

        grLoadBmp(ttmSlot, 0, "HOLIDAY.BMP");

        switch (islandState.holiday) {
            case 1: grDrawSprite(ttmThread->ttmLayer, ttmSlot, 410, 298, 0, 0); break;   // Halloween
            case 2: grDrawSprite(ttmThread->ttmLayer, ttmSlot, 333, 286, 1, 0); break;   // St Patrick
            case 3: grDrawSprite(ttmThread->ttmLayer, ttmSlot, 404, 267, 2, 0); break;   // Christmas
            case 4: grDrawSprite(ttmThread->ttmLayer, ttmSlot, 361, 155, 3, 0); break;   // New year
        }

        grReleaseBmp(ttmSlot,0);
    }
    else {
        ttmThread->isRunning = 0;
    }
}

void islandAnimateClouds(struct TTtmThread *ttmThread) {
    struct TTtmSlot *ttmSlot = ttmThread->ttmSlot;
    grClearScreen(ttmThread->ttmLayer);
    if (islandState.clouds.numClouds > 0) {
        ttmThread->isRunning = 3;
        grLoadBmp(ttmSlot, 0, "BACKGRND.BMP");

        // animate clouds x position
        for (sint32 i=0; i < islandState.clouds.numClouds; i++) {
            sint32 cloudNo = islandState.clouds.cloudNo[i];
            sint32 cloudX = islandState.clouds.xPos[i];
            sint32 cloudY = islandState.clouds.yPos[i];

            if (cloudX > SCREEN_WIDTH + 264) {
                cloudX = -264;
            } else if (cloudX < -264) {
                cloudX = SCREEN_WIDTH + 264;
            }
            else {
                if (islandState.clouds.windDirection) {
                    cloudX -= islandState.clouds.windSpeed[i];
                } else {
                    cloudX += islandState.clouds.windSpeed[i];
                }
            }

            debugMsg("Clouds Pos: %d, %d", cloudX, cloudY);
            if (islandState.clouds.windDirection) {
                grDrawSprite(ttmThread->ttmLayer, ttmSlot, cloudX, cloudY, 15 + cloudNo, 0);
            } else {
                grDrawSpriteFlip(ttmThread->ttmLayer, ttmSlot, cloudX, cloudY, 15 + cloudNo, 0);
            }

            islandState.clouds.xPos[i] = cloudX;
            islandState.clouds.yPos[i] = cloudY;
        }
    } else {
        ttmThread->isRunning = 0;
    }
}
