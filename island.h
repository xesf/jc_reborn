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

#include "mytypes.h"

struct TCloudState {
    sint32 numClouds;
    sint32 windDirection;
    sint32 windSpeed[5];
    sint32 cloudNo[5];
    sint32 xPos[5];
    sint32 yPos[5];
};

struct TIslandState {
    sint32 lowTide;
    sint32 night;
    sint32 raft;
    sint32 holiday;
    sint32 xPos;
    sint32 yPos;
    struct TCloudState clouds;
};

extern struct TIslandState islandState;

void islandInit(struct TTtmThread *ttmThread);
void islandAnimate(struct TTtmThread *ttmThread);
void islandInitHoliday(struct TTtmThread *ttmThread);
void islandAnimateClouds(struct TTtmThread *ttmThread);
