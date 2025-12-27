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

#include "platform.h"
#include "mytypes.h"
#include "graphics.h"
#include "events.h"


static uint32 lastTicks = 0x00ffffff;
static int paused   = 0;
static int maxSpeed = 0;
static int oneFrame = 0;

int evHotKeysEnabled = 0;


static void eventsProcessEvents()
{
    PlatformEvent event;

    while (platformPollEvent(&event)) {

        switch(event.type) {

            case EVENT_KEY_DOWN:

                if (evHotKeysEnabled) {

                    switch (event.data.key.keycode) {

                        case KEY_SPACE:
                            paused = !paused;
                            break;

                        case KEY_M:
                            maxSpeed = !maxSpeed;
                            break;

                        case KEY_RETURN:
                            if (event.data.key.modifiers & KEYMOD_LALT) {
                                grToggleFullScreen();
                                oneFrame = 1;   // to redraw the window // TODO
                            }
                            else {
                                oneFrame = 1;
                            }
                            break;

                        case KEY_ESCAPE:
                            graphicsEnd();
                            exit(255);
                            break;
                        
                        default:
                            break;
                    }
                }
                else {
                    // Normal behaviour : no hot keys, the screen saver
                    // terminates if any key is pressed
                    graphicsEnd();
                    exit(255);
                }
                break;

            case EVENT_WINDOW_REFRESH:
                grRefreshDisplay();
                break;

            case EVENT_QUIT:
                graphicsEnd();
                exit(255);
                break;
            
            default:
                break;
        }
    }
}


void eventsInit()
{
    lastTicks = platformGetTicks();
}


void eventsWaitTick(uint16 delay)
{
    delay *= 20;
    oneFrame = 0;

    eventsProcessEvents();

    while ((paused && !oneFrame)
            || (!maxSpeed && (platformGetTicks() - lastTicks < delay))) {
        platformDelay(5);
        eventsProcessEvents();
    }

    lastTicks = platformGetTicks();
}