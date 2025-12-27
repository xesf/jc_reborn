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

#ifndef PLATFORM_H
#define PLATFORM_H

// Platform macros are defined by CMake build system:
// PLATFORM_WEB, PLATFORM_WINDOWS, PLATFORM_MACOS, PLATFORM_LINUX

#include "mytypes.h"

// Forward declarations
typedef struct PlatformSurface PlatformSurface;
typedef struct PlatformWindow PlatformWindow;
typedef struct PlatformRect PlatformRect;

// Platform rectangle
struct PlatformRect {
    sint16 x;
    sint16 y;
    uint16 w;
    uint16 h;
};

// Key codes (platform-independent)
typedef enum {
    KEY_UNKNOWN = 0,
    KEY_SPACE,
    KEY_RETURN,
    KEY_ESCAPE,
    KEY_M,
    KEY_LALT
} PlatformKeyCode;

// Event types
typedef enum {
    EVENT_NONE = 0,
    EVENT_QUIT,
    EVENT_KEY_DOWN,
    EVENT_KEY_UP,
    EVENT_WINDOW_REFRESH
} PlatformEventType;

// Key modifiers
typedef enum {
    KEYMOD_NONE = 0,
    KEYMOD_LALT = 1 << 0,
    KEYMOD_RALT = 1 << 1,
    KEYMOD_LSHIFT = 1 << 2,
    KEYMOD_RSHIFT = 1 << 3,
    KEYMOD_LCTRL = 1 << 4,
    KEYMOD_RCTRL = 1 << 5
} PlatformKeyMod;

// Platform event
typedef struct {
    PlatformEventType type;
    union {
        struct {
            PlatformKeyCode keycode;
            uint16 modifiers;
        } key;
    } data;
} PlatformEvent;

// Platform initialization and shutdown
int platformInit(void);
void platformShutdown(void);

// Graphics - Window management
PlatformWindow* platformCreateWindow(const char* title, int width, int height, int fullscreen);
void platformDestroyWindow(PlatformWindow* window);
void platformShowCursor(int show);
void platformToggleFullscreen(PlatformWindow* window);
void platformUpdateWindow(PlatformWindow* window);
PlatformSurface* platformGetWindowSurface(PlatformWindow* window);

// Graphics - Surface management
PlatformSurface* platformCreateSurface(int width, int height);
PlatformSurface* platformCreateSurfaceFrom(void* pixels, int width, int height, int pitch);
void platformFreeSurface(PlatformSurface* surface);
void platformLockSurface(PlatformSurface* surface);
void platformUnlockSurface(PlatformSurface* surface);

// Graphics - Blitting and drawing
void platformBlitSurface(PlatformSurface* src, PlatformRect* srcRect,
                        PlatformSurface* dst, PlatformRect* dstRect);
void platformFillRect(PlatformSurface* surface, PlatformRect* rect,
                     uint8 r, uint8 g, uint8 b, uint8 a);
void platformSetColorKey(PlatformSurface* surface, uint8 r, uint8 g, uint8 b);
void platformSetClipRect(PlatformSurface* surface, PlatformRect* rect);
void platformGetClipRect(PlatformSurface* surface, PlatformRect* rect);
uint32 platformMapRGB(PlatformSurface* surface, uint8 r, uint8 g, uint8 b);

// Graphics - Surface access
uint8* platformGetSurfacePixels(PlatformSurface* surface);
int platformGetSurfacePitch(PlatformSurface* surface);
int platformGetSurfaceWidth(PlatformSurface* surface);
int platformGetSurfaceHeight(PlatformSurface* surface);
int platformGetSurfaceBytesPerPixel(PlatformSurface* surface);

// Events
int platformPollEvent(PlatformEvent* event);

// Timing
uint32 platformGetTicks(void);
void platformDelay(uint32 ms);

// Audio
typedef void (*PlatformAudioCallback)(void* userdata, uint8* stream, int len);

typedef struct {
    int freq;
    uint16 format;
    uint8 channels;
    uint16 samples;
    PlatformAudioCallback callback;
    void* userdata;
} PlatformAudioSpec;

int platformInitAudio(void);
void platformCloseAudio(void);
int platformOpenAudio(PlatformAudioSpec* spec);
void platformPauseAudio(int pause);
void platformLockAudio(void);
void platformUnlockAudio(void);
int platformLoadWAV(const char* filename, PlatformAudioSpec* spec,
                    uint8** audio_buf, uint32* audio_len);
void platformFreeWAV(uint8* audio_buf);

// Platform-specific error reporting
const char* platformGetError(void);

#endif // PLATFORM_H
