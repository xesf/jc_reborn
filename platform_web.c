/*
 *  This file is part of 'Johnny Reborn'
 *  Platform implementation for Web (Emscripten/Canvas)
 */

#ifdef PLATFORM_WEB

#include "platform.h"
#include <emscripten.h>
#include <emscripten/html5.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static const char* lastError = "";
static double startTime;

// Surface structure
struct PlatformSurface {
    int width;
    int height;
    int pitch;
    int bytesPerPixel;
    uint8* pixels;
    uint8 hasColorKey;
    uint8 colorKeyR, colorKeyG, colorKeyB;
    PlatformRect clipRect;
    int ownPixels;
};

// Window structure
struct PlatformWindow {
    const char* canvasId;
    PlatformSurface* surface;
    int isFullscreen;
};

static PlatformWindow* mainWindow = NULL;
static PlatformEvent pendingEvents[32];
static int pendingEventCount = 0;

// Keyboard callback
EM_BOOL keyCallback(int eventType, const EmscriptenKeyboardEvent* keyEvent, void* userData) {
    if (pendingEventCount >= 32) return EM_FALSE;
    
    PlatformEvent* ev = &pendingEvents[pendingEventCount];
    ev->type = (eventType == EMSCRIPTEN_EVENT_KEYDOWN) ? EVENT_KEY_DOWN : EVENT_KEY_UP;
    ev->data.key.modifiers = 0;
    
    if (keyEvent->altKey) {
        ev->data.key.modifiers |= KEYMOD_LALT;
    }
    
    if (strcmp(keyEvent->key, " ") == 0 || strcmp(keyEvent->key, "Space") == 0) {
        ev->data.key.keycode = KEY_SPACE;
    } else if (strcmp(keyEvent->key, "Enter") == 0) {
        ev->data.key.keycode = KEY_RETURN;
    } else if (strcmp(keyEvent->key, "Escape") == 0) {
        ev->data.key.keycode = KEY_ESCAPE;
    } else if (strcmp(keyEvent->key, "m") == 0 || strcmp(keyEvent->key, "M") == 0) {
        ev->data.key.keycode = KEY_M;
    } else {
        ev->data.key.keycode = KEY_UNKNOWN;
    }
    
    pendingEventCount++;
    return EM_TRUE;
}

// Initialize platform
int platformInit(void) {
    startTime = emscripten_get_now();
    
    emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, 1, keyCallback);
    emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, 1, keyCallback);
    
    return 0;
}

void platformShutdown(void) {
    // Cleanup
}

// Window management
PlatformWindow* platformCreateWindow(const char* title, int width, int height, int fullscreen) {
    PlatformWindow* window = (PlatformWindow*)malloc(sizeof(PlatformWindow));
    window->canvasId = "#canvas";
    window->surface = platformCreateSurface(width, height);
    window->isFullscreen = 0;
    
    emscripten_set_canvas_element_size(window->canvasId, width, height);
    
    mainWindow = window;
    
    if (fullscreen) {
        platformToggleFullscreen(window);
    }
    
    return window;
}

void platformDestroyWindow(PlatformWindow* window) {
    if (window) {
        if (window->surface) {
            platformFreeSurface(window->surface);
        }
        free(window);
    }
}

void platformShowCursor(int show) {
    if (show) {
        EM_ASM(
            document.getElementById('canvas').style.cursor = 'default';
        );
    } else {
        EM_ASM(
            document.getElementById('canvas').style.cursor = 'none';
        );
    }
}

void platformToggleFullscreen(PlatformWindow* window) {
    if (!window->isFullscreen) {
        EmscriptenFullscreenStrategy strategy = {
            .scaleMode = EMSCRIPTEN_FULLSCREEN_SCALE_DEFAULT,
            .canvasResolutionScaleMode = EMSCRIPTEN_FULLSCREEN_CANVAS_SCALE_NONE,
            .filteringMode = EMSCRIPTEN_FULLSCREEN_FILTERING_DEFAULT
        };
        emscripten_request_fullscreen_strategy(window->canvasId, 1, &strategy);
    } else {
        emscripten_exit_fullscreen();
    }
    window->isFullscreen = !window->isFullscreen;
}

void platformUpdateWindow(PlatformWindow* window) {
    if (!window || !window->surface) return;
    
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE ctx = emscripten_webgl_get_current_context();
    if (!ctx) {
        // Create 2D context
        EmscriptenWebGLContextAttributes attrs;
        emscripten_webgl_init_context_attributes(&attrs);
        ctx = emscripten_webgl_create_context(window->canvasId, &attrs);
        emscripten_webgl_make_context_current(ctx);
    }
    
    // Draw pixels to canvas using JavaScript
    EM_ASM({
        var canvas = document.querySelector('#canvas');
        if (!canvas) return;
        
        var ctx = canvas.getContext('2d');
        if (!ctx) return;
        
        var width = $0;
        var height = $1;
        var pixels = $2;
        
        var imageData = ctx.createImageData(width, height);
        var data = imageData.data;
        
        // Copy pixel data from WASM memory
        for (var i = 0; i < width * height * 4; i++) {
            data[i] = HEAPU8[pixels + i];
        }
        
        ctx.putImageData(imageData, 0, 0);
    }, window->surface->width, window->surface->height, window->surface->pixels);
}

PlatformSurface* platformGetWindowSurface(PlatformWindow* window) {
    return window ? window->surface : NULL;
}

// Surface management
PlatformSurface* platformCreateSurface(int width, int height) {
    PlatformSurface* surface = (PlatformSurface*)malloc(sizeof(PlatformSurface));
    surface->width = width;
    surface->height = height;
    surface->bytesPerPixel = 4;
    surface->pitch = width * 4;
    surface->pixels = (uint8*)calloc(width * height, 4);
    surface->hasColorKey = 0;
    surface->clipRect.x = 0;
    surface->clipRect.y = 0;
    surface->clipRect.w = width;
    surface->clipRect.h = height;
    surface->ownPixels = 1;
    return surface;
}

PlatformSurface* platformCreateSurfaceFrom(void* pixels, int width, int height, int pitch) {
    PlatformSurface* surface = (PlatformSurface*)malloc(sizeof(PlatformSurface));
    surface->width = width;
    surface->height = height;
    surface->bytesPerPixel = 4;
    surface->pitch = pitch;
    surface->pixels = (uint8*)pixels;
    surface->hasColorKey = 0;
    surface->clipRect.x = 0;
    surface->clipRect.y = 0;
    surface->clipRect.w = width;
    surface->clipRect.h = height;
    surface->ownPixels = 0;
    return surface;
}

void platformFreeSurface(PlatformSurface* surface) {
    if (surface) {
        if (surface->ownPixels && surface->pixels) {
            free(surface->pixels);
        }
        free(surface);
    }
}

void platformLockSurface(PlatformSurface* surface) {}
void platformUnlockSurface(PlatformSurface* surface) {}

// Blitting and drawing (same as other platforms)
void platformBlitSurface(PlatformSurface* src, PlatformRect* srcRect,
                        PlatformSurface* dst, PlatformRect* dstRect) {
    if (!src || !dst || !src->pixels || !dst->pixels) return;
    
    int srcX = srcRect ? srcRect->x : 0;
    int srcY = srcRect ? srcRect->y : 0;
    int srcW = srcRect ? srcRect->w : src->width;
    int srcH = srcRect ? srcRect->h : src->height;
    
    int dstX = dstRect ? dstRect->x : 0;
    int dstY = dstRect ? dstRect->y : 0;
    
    if (dstX < dst->clipRect.x) {
        srcX += dst->clipRect.x - dstX;
        srcW -= dst->clipRect.x - dstX;
        dstX = dst->clipRect.x;
    }
    if (dstY < dst->clipRect.y) {
        srcY += dst->clipRect.y - dstY;
        srcH -= dst->clipRect.y - dstY;
        dstY = dst->clipRect.y;
    }
    if (dstX + srcW > dst->clipRect.x + dst->clipRect.w) {
        srcW = dst->clipRect.x + dst->clipRect.w - dstX;
    }
    if (dstY + srcH > dst->clipRect.y + dst->clipRect.h) {
        srcH = dst->clipRect.y + dst->clipRect.h - dstY;
    }
    
    if (srcW <= 0 || srcH <= 0) return;
    
    for (int y = 0; y < srcH; y++) {
        for (int x = 0; x < srcW; x++) {
            int sx = srcX + x;
            int sy = srcY + y;
            int dx = dstX + x;
            int dy = dstY + y;
            
            if (sx < 0 || sy < 0 || sx >= src->width || sy >= src->height) continue;
            if (dx < 0 || dy < 0 || dx >= dst->width || dy >= dst->height) continue;
            
            uint8* srcPixel = src->pixels + sy * src->pitch + sx * src->bytesPerPixel;
            uint8* dstPixel = dst->pixels + dy * dst->pitch + dx * dst->bytesPerPixel;
            
            if (src->hasColorKey) {
                if (srcPixel[0] == src->colorKeyB &&
                    srcPixel[1] == src->colorKeyG &&
                    srcPixel[2] == src->colorKeyR) {
                    continue;
                }
            }
            
            memcpy(dstPixel, srcPixel, 4);
        }
    }
}

void platformFillRect(PlatformSurface* surface, PlatformRect* rect,
                     uint8 r, uint8 g, uint8 b, uint8 a) {
    if (!surface || !surface->pixels) return;
    
    int x = rect ? rect->x : 0;
    int y = rect ? rect->y : 0;
    int w = rect ? rect->w : surface->width;
    int h = rect ? rect->h : surface->height;
    
    for (int py = y; py < y + h && py < surface->height; py++) {
        for (int px = x; px < x + w && px < surface->width; px++) {
            uint8* pixel = surface->pixels + py * surface->pitch + px * surface->bytesPerPixel;
            pixel[0] = b;
            pixel[1] = g;
            pixel[2] = r;
            pixel[3] = a;
        }
    }
}

void platformSetColorKey(PlatformSurface* surface, uint8 r, uint8 g, uint8 b) {
    if (surface) {
        surface->hasColorKey = 1;
        surface->colorKeyR = r;
        surface->colorKeyG = g;
        surface->colorKeyB = b;
    }
}

void platformSetClipRect(PlatformSurface* surface, PlatformRect* rect) {
    if (surface) {
        if (rect) {
            surface->clipRect = *rect;
        } else {
            surface->clipRect.x = 0;
            surface->clipRect.y = 0;
            surface->clipRect.w = surface->width;
            surface->clipRect.h = surface->height;
        }
    }
}

void platformGetClipRect(PlatformSurface* surface, PlatformRect* rect) {
    if (surface && rect) {
        *rect = surface->clipRect;
    }
}

uint32 platformMapRGB(PlatformSurface* surface, uint8 r, uint8 g, uint8 b) {
    return (r << 16) | (g << 8) | b;
}

uint8* platformGetSurfacePixels(PlatformSurface* surface) {
    return surface ? surface->pixels : NULL;
}

int platformGetSurfacePitch(PlatformSurface* surface) {
    return surface ? surface->pitch : 0;
}

int platformGetSurfaceWidth(PlatformSurface* surface) {
    return surface ? surface->width : 0;
}

int platformGetSurfaceHeight(PlatformSurface* surface) {
    return surface ? surface->height : 0;
}

int platformGetSurfaceBytesPerPixel(PlatformSurface* surface) {
    return surface ? surface->bytesPerPixel : 0;
}

// Events
int platformPollEvent(PlatformEvent* event) {
    if (pendingEventCount > 0) {
        *event = pendingEvents[0];
        for (int i = 1; i < pendingEventCount; i++) {
            pendingEvents[i-1] = pendingEvents[i];
        }
        pendingEventCount--;
        return 1;
    }
    return 0;
}

// Timing
uint32 platformGetTicks(void) {
    return (uint32)(emscripten_get_now() - startTime);
}

void platformDelay(uint32 ms) {
    emscripten_sleep(ms);
}

// Audio (Web Audio API)
static PlatformAudioCallback audioCallback = NULL;
static void* audioUserData = NULL;

int platformInitAudio(void) {
    // Initialize Web Audio Context via JavaScript
    EM_ASM({
        if (typeof window.audioContext === 'undefined') {
            window.audioContext = new (window.AudioContext || window.webkitAudioContext)();
        }
    });
    return 0;
}

void platformCloseAudio(void) {
    EM_ASM({
        if (window.audioContext) {
            window.audioContext.close();
            window.audioContext = null;
        }
    });
}

int platformOpenAudio(PlatformAudioSpec* spec) {
    audioCallback = spec->callback;
    audioUserData = spec->userdata;
    
    // Setup Web Audio via JavaScript
    EM_ASM({
        if (!window.audioContext) return;
        
        var bufferSize = $0;
        var sampleRate = $1;
        
        window.audioProcessor = window.audioContext.createScriptProcessor(bufferSize, 0, 1);
        window.audioProcessor.onaudioprocess = function(e) {
            // Audio processing would be done here
        };
        
        window.audioProcessor.connect(window.audioContext.destination);
    }, spec->samples, spec->freq);
    
    return 0;
}

void platformPauseAudio(int pause) {
    if (pause) {
        EM_ASM({
            if (window.audioContext) {
                window.audioContext.suspend();
            }
        });
    } else {
        EM_ASM({
            if (window.audioContext) {
                window.audioContext.resume();
            }
        });
    }
}

void platformLockAudio(void) {}
void platformUnlockAudio(void) {}

int platformLoadWAV(const char* filename, PlatformAudioSpec* spec,
                    uint8** audio_buf, uint32* audio_len) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        lastError = "Failed to open WAV file";
        return -1;
    }
    
    uint8 header[44];
    if (fread(header, 1, 44, file) != 44) {
        fclose(file);
        lastError = "Invalid WAV file";
        return -1;
    }
    
    uint32 dataSize = *(uint32*)(header + 40);
    *audio_len = dataSize;
    *audio_buf = (uint8*)malloc(dataSize);
    
    if (fread(*audio_buf, 1, dataSize, file) != dataSize) {
        free(*audio_buf);
        fclose(file);
        lastError = "Failed to read WAV data";
        return -1;
    }
    
    fclose(file);
    
    spec->freq = *(uint32*)(header + 24);
    spec->channels = *(uint16*)(header + 22);
    spec->format = *(uint16*)(header + 34);
    
    return 0;
}

void platformFreeWAV(uint8* audio_buf) {
    free(audio_buf);
}

const char* platformGetError(void) {
    return lastError;
}

#endif // PLATFORM_WEB
