/*
 *  This file is part of 'Johnny Reborn'
 *  Platform implementation for macOS (Cocoa)
 */

#ifdef PLATFORM_MACOS

#include "platform.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <mach/mach_time.h>
#include <AudioToolbox/AudioToolbox.h>
#include <Cocoa/Cocoa.h>

static const char* lastError = "";
static mach_timebase_info_data_t timebaseInfo;
static uint64_t startTime;

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
    int ownPixels;  // 1 if we allocated pixels, 0 if external
};

// Window structure
@interface JCRebornWindow : NSWindow
@end

@implementation JCRebornWindow
- (BOOL)canBecomeKeyWindow { return YES; }
- (BOOL)canBecomeMainWindow { return YES; }
@end

@interface JCRebornView : NSView {
    @public
    PlatformSurface* surface;
}
@end

@implementation JCRebornView
- (void)drawRect:(NSRect)dirtyRect {
    if (!surface || !surface->pixels) return;
    
    CGContextRef context = [[NSGraphicsContext currentContext] CGContext];
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    
    // Use BGRA format to match the pixel data format (B=0, G=1, R=2, A=3)
    CGContextRef bitmapContext = CGBitmapContextCreate(
        surface->pixels,
        surface->width,
        surface->height,
        8,
        surface->pitch,
        colorSpace,
        kCGImageAlphaNoneSkipFirst | kCGBitmapByteOrder32Little
    );
    
    CGImageRef image = CGBitmapContextCreateImage(bitmapContext);
    CGRect rect = CGRectMake(0, 0, surface->width, surface->height);
    
    // Draw image directly without flipping (image data is already correct orientation)
    CGContextDrawImage(context, rect, image);
    
    CGImageRelease(image);
    CGContextRelease(bitmapContext);
    CGColorSpaceRelease(colorSpace);
}

- (BOOL)acceptsFirstResponder { return YES; }
@end

struct PlatformWindow {
    JCRebornWindow* nsWindow;
    JCRebornView* view;
    PlatformSurface* surface;
    int isFullscreen;
};

static NSMutableArray* eventQueue;
static PlatformWindow* mainWindow = NULL;

// Initialize platform
int platformInit(void) {
    @autoreleasepool {
        [NSApplication sharedApplication];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
        
        eventQueue = [[NSMutableArray alloc] init];
        
        // Initialize timing
        mach_timebase_info(&timebaseInfo);
        startTime = mach_absolute_time();
        
        return 0;
    }
}

void platformShutdown(void) {
    @autoreleasepool {
        [eventQueue release];
        [NSApp terminate:nil];
    }
}

// Window management
PlatformWindow* platformCreateWindow(const char* title, int width, int height, int fullscreen) {
    @autoreleasepool {
        PlatformWindow* window = (PlatformWindow*)malloc(sizeof(PlatformWindow));
        
        NSRect frame = NSMakeRect(0, 0, width, height);
        NSWindowStyleMask style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | 
                                 NSWindowStyleMaskMiniaturizable;
        
        window->nsWindow = [[JCRebornWindow alloc]
            initWithContentRect:frame
            styleMask:style
            backing:NSBackingStoreBuffered
            defer:NO];
        
        [window->nsWindow setTitle:[NSString stringWithUTF8String:title]];
        [window->nsWindow center];
        [window->nsWindow makeKeyAndOrderFront:nil];
        
        window->view = [[JCRebornView alloc] initWithFrame:frame];
        [window->nsWindow setContentView:window->view];
        
        window->surface = platformCreateSurface(width, height);
        window->view->surface = window->surface;
        window->isFullscreen = 0;
        
        mainWindow = window;
        
        if (fullscreen) {
            platformToggleFullscreen(window);
        }
        
        [NSApp activateIgnoringOtherApps:YES];
        
        return window;
    }
}

void platformDestroyWindow(PlatformWindow* window) {
    @autoreleasepool {
        if (window) {
            if (window->surface) {
                platformFreeSurface(window->surface);
            }
            [window->view release];
            [window->nsWindow close];
            [window->nsWindow release];
            free(window);
        }
    }
}

void platformShowCursor(int show) {
    @autoreleasepool {
        if (show) {
            [NSCursor unhide];
        } else {
            [NSCursor hide];
        }
    }
}

void platformToggleFullscreen(PlatformWindow* window) {
    @autoreleasepool {
        [window->nsWindow toggleFullScreen:nil];
        window->isFullscreen = !window->isFullscreen;
    }
}

void platformUpdateWindow(PlatformWindow* window) {
    @autoreleasepool {
        [window->view setNeedsDisplay:YES];
        [window->view display];
    }
}

PlatformSurface* platformGetWindowSurface(PlatformWindow* window) {
    return window->surface;
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
    // No-op on macOS
}

void platformUnlockSurface(PlatformSurface* surface) {
    // No-op on macOS
}

// Blitting and drawing
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
    @autoreleasepool {
        NSEvent* nsEvent = [NSApp nextEventMatchingMask:NSEventMaskAny
                                              untilDate:[NSDate distantPast]
                                                 inMode:NSDefaultRunLoopMode
                                                dequeue:YES];
        
        if (!nsEvent) return 0;
        
        [NSApp sendEvent:nsEvent];
        
        event->type = EVENT_NONE;
        
        switch ([nsEvent type]) {
            case NSEventTypeKeyDown: {
                event->type = EVENT_KEY_DOWN;
                NSString* chars = [nsEvent charactersIgnoringModifiers];
                if ([chars length] > 0) {
                    unichar c = [chars characterAtIndex:0];
                    switch (c) {
                        case ' ': event->data.key.keycode = KEY_SPACE; break;
                        case '\r': case '\n': event->data.key.keycode = KEY_RETURN; break;
                        case 27: event->data.key.keycode = KEY_ESCAPE; break;
                        case 'm': case 'M': event->data.key.keycode = KEY_M; break;
                        default: event->data.key.keycode = KEY_UNKNOWN; break;
                    }
                }
                NSEventModifierFlags flags = [nsEvent modifierFlags];
                event->data.key.modifiers = 0;
                if (flags & NSEventModifierFlagOption) {
                    event->data.key.modifiers |= KEYMOD_LALT;
                }
                return 1;
            }
            
            case NSEventTypeKeyUp:
                event->type = EVENT_KEY_UP;
                return 1;
            
            default:
                break;
        }
        
        return 0;
    }
}

// Timing
uint32 platformGetTicks(void) {
    uint64_t elapsed = mach_absolute_time() - startTime;
    return (uint32)((elapsed * timebaseInfo.numer) / (timebaseInfo.denom * 1000000));
}

void platformDelay(uint32 ms) {
    usleep(ms * 1000);
}

// Audio
static AudioQueueRef audioQueue = NULL;
static PlatformAudioCallback audioCallback = NULL;
static void* audioUserData = NULL;
static AudioQueueBufferRef audioBuffers[3];

static void audioOutputCallback(void* userData, AudioQueueRef queue, AudioQueueBufferRef buffer) {
    if (audioCallback) {
        audioCallback(audioUserData, (uint8*)buffer->mAudioData, buffer->mAudioDataBytesCapacity);
    } else {
        memset(buffer->mAudioData, 0, buffer->mAudioDataBytesCapacity);
    }
    AudioQueueEnqueueBuffer(queue, buffer, 0, NULL);
}

int platformInitAudio(void) {
    return 0;
}

void platformCloseAudio(void) {
    if (audioQueue) {
        AudioQueueStop(audioQueue, true);
        AudioQueueDispose(audioQueue, true);
        audioQueue = NULL;
    }
}

int platformOpenAudio(PlatformAudioSpec* spec) {
    AudioStreamBasicDescription format;
    format.mSampleRate = spec->freq;
    format.mFormatID = kAudioFormatLinearPCM;
    // 8-bit audio is unsigned, not signed (0-255, with 128 as silence)
    format.mFormatFlags = kLinearPCMFormatFlagIsPacked;
    format.mBitsPerChannel = 8;
    format.mChannelsPerFrame = spec->channels;
    format.mBytesPerFrame = spec->channels;
    format.mFramesPerPacket = 1;
    format.mBytesPerPacket = spec->channels;
    
    audioCallback = spec->callback;
    audioUserData = spec->userdata;
    
    OSStatus status = AudioQueueNewOutput(&format, audioOutputCallback, NULL,
                                         NULL, NULL, 0, &audioQueue);
    if (status != 0) {
        lastError = "Failed to create audio queue";
        return -1;
    }
    
    int bufferSize = spec->samples * spec->channels;
    for (int i = 0; i < 3; i++) {
        AudioQueueAllocateBuffer(audioQueue, bufferSize, &audioBuffers[i]);
        audioBuffers[i]->mAudioDataByteSize = bufferSize;
        audioOutputCallback(NULL, audioQueue, audioBuffers[i]);
    }
    
    return 0;
}

void platformPauseAudio(int pause) {
    if (audioQueue) {
        if (pause) {
            AudioQueuePause(audioQueue);
        } else {
            AudioQueueStart(audioQueue, NULL);
        }
    }
}

void platformLockAudio(void) {
    // Not needed for AudioQueue
}

void platformUnlockAudio(void) {
    // Not needed for AudioQueue
}

int platformLoadWAV(const char* filename, PlatformAudioSpec* spec,
                    uint8** audio_buf, uint32* audio_len) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        lastError = "Failed to open WAV file";
        return -1;
    }
    
    // Simple WAV parser (assumes PCM, 8-bit, mono)
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

#endif // PLATFORM_MACOS
