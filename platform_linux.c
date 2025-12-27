/*
 *  This file is part of 'Johnny Reborn'
 *  Platform implementation for Linux (X11)
 */

#ifdef PLATFORM_LINUX

#include "platform.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <alsa/asoundlib.h>

static const char* lastError = "";
static Display* display = NULL;
static struct timespec startTime;

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
    Window window;
    GC gc;
    XImage* ximage;
    PlatformSurface* surface;
    int isFullscreen;
    Atom wmDeleteWindow;
};

static PlatformWindow* mainWindow = NULL;

// Initialize platform
int platformInit(void) {
    display = XOpenDisplay(NULL);
    if (!display) {
        lastError = "Failed to open X display";
        return -1;
    }
    
    clock_gettime(CLOCK_MONOTONIC, &startTime);
    return 0;
}

void platformShutdown(void) {
    if (display) {
        XCloseDisplay(display);
        display = NULL;
    }
}

// Window management
PlatformWindow* platformCreateWindow(const char* title, int width, int height, int fullscreen) {
    if (!display) return NULL;
    
    PlatformWindow* window = (PlatformWindow*)malloc(sizeof(PlatformWindow));
    int screen = DefaultScreen(display);
    
    window->window = XCreateSimpleWindow(display, RootWindow(display, screen),
                                        0, 0, width, height, 0,
                                        BlackPixel(display, screen),
                                        BlackPixel(display, screen));
    
    XSelectInput(display, window->window,
                KeyPressMask | KeyReleaseMask | ExposureMask | 
                StructureNotifyMask | FocusChangeMask);
    
    XStoreName(display, window->window, title);
    
    window->wmDeleteWindow = XInternAtom(display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(display, window->window, &window->wmDeleteWindow, 1);
    
    XMapWindow(display, window->window);
    XFlush(display);
    
    window->gc = XCreateGC(display, window->window, 0, NULL);
    
    window->surface = platformCreateSurface(width, height);
    window->isFullscreen = 0;
    
    Visual* visual = DefaultVisual(display, screen);
    int depth = DefaultDepth(display, screen);
    
    window->ximage = XCreateImage(display, visual, depth, ZPixmap, 0,
                                  (char*)window->surface->pixels,
                                  width, height, 32, window->surface->pitch);
    
    mainWindow = window;
    
    if (fullscreen) {
        platformToggleFullscreen(window);
    }
    
    return window;
}

void platformDestroyWindow(PlatformWindow* window) {
    if (window) {
        if (window->ximage) {
            window->ximage->data = NULL;  // Prevent XDestroyImage from freeing our pixels
            XDestroyImage(window->ximage);
        }
        if (window->gc) {
            XFreeGC(display, window->gc);
        }
        if (window->surface) {
            platformFreeSurface(window->surface);
        }
        XDestroyWindow(display, window->window);
        free(window);
    }
}

void platformShowCursor(int show) {
    if (!mainWindow) return;
    
    if (!show) {
        Cursor invisibleCursor;
        Pixmap bitmapNoData;
        XColor black;
        static char noData[] = { 0,0,0,0,0,0,0,0 };
        black.red = black.green = black.blue = 0;
        
        bitmapNoData = XCreateBitmapFromData(display, mainWindow->window, noData, 8, 8);
        invisibleCursor = XCreatePixmapCursor(display, bitmapNoData, bitmapNoData,
                                             &black, &black, 0, 0);
        XDefineCursor(display, mainWindow->window, invisibleCursor);
        XFreeCursor(display, invisibleCursor);
        XFreePixmap(display, bitmapNoData);
    } else {
        XUndefineCursor(display, mainWindow->window);
    }
}

void platformToggleFullscreen(PlatformWindow* window) {
    Atom wmState = XInternAtom(display, "_NET_WM_STATE", False);
    Atom fullscreen = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", False);
    
    XEvent xev;
    memset(&xev, 0, sizeof(xev));
    xev.type = ClientMessage;
    xev.xclient.window = window->window;
    xev.xclient.message_type = wmState;
    xev.xclient.format = 32;
    xev.xclient.data.l[0] = window->isFullscreen ? 0 : 1;
    xev.xclient.data.l[1] = fullscreen;
    xev.xclient.data.l[2] = 0;
    
    XSendEvent(display, DefaultRootWindow(display), False,
              SubstructureRedirectMask | SubstructureNotifyMask, &xev);
    
    window->isFullscreen = !window->isFullscreen;
    XFlush(display);
}

void platformUpdateWindow(PlatformWindow* window) {
    if (!window || !window->ximage) return;
    
    XPutImage(display, window->window, window->gc, window->ximage,
             0, 0, 0, 0, window->surface->width, window->surface->height);
    XFlush(display);
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

void platformLockSurface(PlatformSurface* surface) {
    // No-op on Linux
}

void platformUnlockSurface(PlatformSurface* surface) {
    // No-op on Linux
}

// Blitting and drawing (same implementation as macOS)
void platformBlitSurface(PlatformSurface* src, PlatformRect* srcRect,
                        PlatformSurface* dst, PlatformRect* dstRect) {
    if (!src || !dst || !src->pixels || !dst->pixels) return;
    
    int srcX = srcRect ? srcRect->x : 0;
    int srcY = srcRect ? srcRect->y : 0;
    int srcW = srcRect ? srcRect->w : src->width;
    int srcH = srcRect ? srcRect->h : src->height;
    
    int dstX = dstRect ? dstRect->x : 0;
    int dstY = dstRect ? dstRect->y : 0;
    
    // Clip to destination clip rect
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
            
            // Check color key
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

// Surface access
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
    if (!display) return 0;
    
    if (!XPending(display)) return 0;
    
    XEvent xev;
    XNextEvent(display, &xev);
    
    event->type = EVENT_NONE;
    
    switch (xev.type) {
        case KeyPress: {
            event->type = EVENT_KEY_DOWN;
            KeySym keysym = XLookupKeysym(&xev.xkey, 0);
            
            switch (keysym) {
                case XK_space: event->data.key.keycode = KEY_SPACE; break;
                case XK_Return: event->data.key.keycode = KEY_RETURN; break;
                case XK_Escape: event->data.key.keycode = KEY_ESCAPE; break;
                case XK_m: case XK_M: event->data.key.keycode = KEY_M; break;
                default: event->data.key.keycode = KEY_UNKNOWN; break;
            }
            
            event->data.key.modifiers = 0;
            if (xev.xkey.state & Mod1Mask) {
                event->data.key.modifiers |= KEYMOD_LALT;
            }
            return 1;
        }
        
        case KeyRelease:
            event->type = EVENT_KEY_UP;
            return 1;
        
        case Expose:
            event->type = EVENT_WINDOW_REFRESH;
            return 1;
        
        case ClientMessage:
            if (mainWindow && (Atom)xev.xclient.data.l[0] == mainWindow->wmDeleteWindow) {
                event->type = EVENT_QUIT;
                return 1;
            }
            break;
    }
    
    return 0;
}

// Timing
uint32 platformGetTicks(void) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    
    uint64_t elapsed_ns = (now.tv_sec - startTime.tv_sec) * 1000000000LL +
                         (now.tv_nsec - startTime.tv_nsec);
    return (uint32)(elapsed_ns / 1000000);
}

void platformDelay(uint32 ms) {
    usleep(ms * 1000);
}

// Audio
static snd_pcm_t* pcmHandle = NULL;
static PlatformAudioCallback audioCallback = NULL;
static void* audioUserData = NULL;
static uint8* audioBuffer = NULL;
static int audioBufferSize = 0;
static pthread_t audioThread;
static int audioThreadRunning = 0;

static void* audioThreadFunc(void* arg) {
    while (audioThreadRunning) {
        if (audioCallback) {
            audioCallback(audioUserData, audioBuffer, audioBufferSize);
            snd_pcm_writei(pcmHandle, audioBuffer, audioBufferSize);
        } else {
            usleep(10000);
        }
    }
    return NULL;
}

int platformInitAudio(void) {
    return 0;
}

void platformCloseAudio(void) {
    if (audioThreadRunning) {
        audioThreadRunning = 0;
        pthread_join(audioThread, NULL);
    }
    
    if (pcmHandle) {
        snd_pcm_drain(pcmHandle);
        snd_pcm_close(pcmHandle);
        pcmHandle = NULL;
    }
    
    if (audioBuffer) {
        free(audioBuffer);
        audioBuffer = NULL;
    }
}

int platformOpenAudio(PlatformAudioSpec* spec) {
    int err;
    
    err = snd_pcm_open(&pcmHandle, "default", SND_PCM_STREAM_PLAYBACK, 0);
    if (err < 0) {
        lastError = "Failed to open ALSA device";
        return -1;
    }
    
    snd_pcm_hw_params_t* params;
    snd_pcm_hw_params_alloca(&params);
    snd_pcm_hw_params_any(pcmHandle, params);
    snd_pcm_hw_params_set_access(pcmHandle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(pcmHandle, params, SND_PCM_FORMAT_U8);
    snd_pcm_hw_params_set_channels(pcmHandle, params, spec->channels);
    snd_pcm_hw_params_set_rate_near(pcmHandle, params, (unsigned int*)&spec->freq, 0);
    
    err = snd_pcm_hw_params(pcmHandle, params);
    if (err < 0) {
        lastError = "Failed to set ALSA parameters";
        snd_pcm_close(pcmHandle);
        return -1;
    }
    
    audioCallback = spec->callback;
    audioUserData = spec->userdata;
    audioBufferSize = spec->samples * spec->channels;
    audioBuffer = (uint8*)malloc(audioBufferSize);
    
    audioThreadRunning = 1;
    pthread_create(&audioThread, NULL, audioThreadFunc, NULL);
    
    return 0;
}

void platformPauseAudio(int pause) {
    if (pcmHandle) {
        if (pause) {
            snd_pcm_pause(pcmHandle, 1);
        } else {
            snd_pcm_pause(pcmHandle, 0);
        }
    }
}

void platformLockAudio(void) {
    // Not implemented for ALSA
}

void platformUnlockAudio(void) {
    // Not implemented for ALSA
}

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

#endif // PLATFORM_LINUX
