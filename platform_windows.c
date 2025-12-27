/*
 *  This file is part of 'Johnny Reborn'
 *  Platform implementation for Windows (Win32)
 */

#ifdef PLATFORM_WINDOWS

#include "platform.h"
#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static const char* lastError = "";
static LARGE_INTEGER performanceFreq;
static LARGE_INTEGER startTime;

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
    HWND hwnd;
    HDC hdc;
    BITMAPINFO bitmapInfo;
    PlatformSurface* surface;
    int isFullscreen;
    WINDOWPLACEMENT windowPlacement;
};

static PlatformWindow* mainWindow = NULL;
static PlatformEvent pendingEvents[32];
static int pendingEventCount = 0;

// Window procedure
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CLOSE:
            if (pendingEventCount < 32) {
                pendingEvents[pendingEventCount].type = EVENT_QUIT;
                pendingEventCount++;
            }
            return 0;
        
        case WM_KEYDOWN:
            if (pendingEventCount < 32) {
                PlatformEvent* ev = &pendingEvents[pendingEventCount];
                ev->type = EVENT_KEY_DOWN;
                ev->data.key.modifiers = 0;
                
                if (GetKeyState(VK_MENU) & 0x8000) {
                    ev->data.key.modifiers |= KEYMOD_LALT;
                }
                
                switch (wParam) {
                    case VK_SPACE: ev->data.key.keycode = KEY_SPACE; break;
                    case VK_RETURN: ev->data.key.keycode = KEY_RETURN; break;
                    case VK_ESCAPE: ev->data.key.keycode = KEY_ESCAPE; break;
                    case 'M': ev->data.key.keycode = KEY_M; break;
                    default: ev->data.key.keycode = KEY_UNKNOWN; break;
                }
                
                pendingEventCount++;
            }
            return 0;
        
        case WM_KEYUP:
            if (pendingEventCount < 32) {
                pendingEvents[pendingEventCount].type = EVENT_KEY_UP;
                pendingEventCount++;
            }
            return 0;
        
        case WM_PAINT:
            if (mainWindow) {
                platformUpdateWindow(mainWindow);
            }
            ValidateRect(hwnd, NULL);
            return 0;
    }
    
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// Initialize platform
int platformInit(void) {
    QueryPerformanceFrequency(&performanceFreq);
    QueryPerformanceCounter(&startTime);
    return 0;
}

void platformShutdown(void) {
    // Cleanup
}

// Window management
PlatformWindow* platformCreateWindow(const char* title, int width, int height, int fullscreen) {
    WNDCLASSA wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = "JCRebornWindow";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    
    RegisterClassA(&wc);
    
    PlatformWindow* window = (PlatformWindow*)malloc(sizeof(PlatformWindow));
    
    DWORD style = WS_OVERLAPPEDWINDOW;
    RECT rect = {0, 0, width, height};
    AdjustWindowRect(&rect, style, FALSE);
    
    window->hwnd = CreateWindowExA(
        0,
        "JCRebornWindow",
        title,
        style,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left,
        rect.bottom - rect.top,
        NULL, NULL,
        GetModuleHandle(NULL),
        NULL
    );
    
    if (!window->hwnd) {
        free(window);
        return NULL;
    }
    
    window->hdc = GetDC(window->hwnd);
    window->surface = platformCreateSurface(width, height);
    window->isFullscreen = 0;
    
    // Setup bitmap info
    memset(&window->bitmapInfo, 0, sizeof(BITMAPINFO));
    window->bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    window->bitmapInfo.bmiHeader.biWidth = width;
    window->bitmapInfo.bmiHeader.biHeight = -height;  // Negative for top-down
    window->bitmapInfo.bmiHeader.biPlanes = 1;
    window->bitmapInfo.bmiHeader.biBitCount = 32;
    window->bitmapInfo.bmiHeader.biCompression = BI_RGB;
    
    ShowWindow(window->hwnd, SW_SHOW);
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
        if (window->hdc) {
            ReleaseDC(window->hwnd, window->hdc);
        }
        if (window->hwnd) {
            DestroyWindow(window->hwnd);
        }
        free(window);
    }
}

void platformShowCursor(int show) {
    ShowCursor(show ? TRUE : FALSE);
}

void platformToggleFullscreen(PlatformWindow* window) {
    if (!window->isFullscreen) {
        GetWindowPlacement(window->hwnd, &window->windowPlacement);
        
        DWORD style = GetWindowLong(window->hwnd, GWL_STYLE);
        style &= ~WS_OVERLAPPEDWINDOW;
        SetWindowLong(window->hwnd, GWL_STYLE, style);
        
        MONITORINFO mi = { sizeof(mi) };
        GetMonitorInfo(MonitorFromWindow(window->hwnd, MONITOR_DEFAULTTOPRIMARY), &mi);
        
        SetWindowPos(window->hwnd, HWND_TOP,
                    mi.rcMonitor.left, mi.rcMonitor.top,
                    mi.rcMonitor.right - mi.rcMonitor.left,
                    mi.rcMonitor.bottom - mi.rcMonitor.top,
                    SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    } else {
        DWORD style = GetWindowLong(window->hwnd, GWL_STYLE);
        style |= WS_OVERLAPPEDWINDOW;
        SetWindowLong(window->hwnd, GWL_STYLE, style);
        
        SetWindowPlacement(window->hwnd, &window->windowPlacement);
        SetWindowPos(window->hwnd, NULL, 0, 0, 0, 0,
                    SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                    SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
    
    window->isFullscreen = !window->isFullscreen;
}

void platformUpdateWindow(PlatformWindow* window) {
    if (!window || !window->surface) return;
    
    // Get the client area size
    RECT clientRect;
    GetClientRect(window->hwnd, &clientRect);
    int clientWidth = clientRect.right - clientRect.left;
    int clientHeight = clientRect.bottom - clientRect.top;
    
    // Calculate aspect-ratio preserving dimensions
    float surfaceAspect = (float)window->surface->width / (float)window->surface->height;
    float windowAspect = (float)clientWidth / (float)clientHeight;
    
    int destWidth, destHeight, destX, destY;
    
    if (windowAspect > surfaceAspect) {
        // Window is wider than surface - fit to height
        destHeight = clientHeight;
        destWidth = (int)(destHeight * surfaceAspect);
        destX = (clientWidth - destWidth) / 2;
        destY = 0;
    } else {
        // Window is taller than surface - fit to width
        destWidth = clientWidth;
        destHeight = (int)(destWidth / surfaceAspect);
        destX = 0;
        destY = (clientHeight - destHeight) / 2;
    }
    
    // Fill borders with black if needed
    if (destX > 0 || destY > 0) {
        HBRUSH blackBrush = (HBRUSH)GetStockObject(BLACK_BRUSH);
        if (destX > 0) {
            // Left border
            RECT leftRect = {0, 0, destX, clientHeight};
            FillRect(window->hdc, &leftRect, blackBrush);
            // Right border
            RECT rightRect = {destX + destWidth, 0, clientWidth, clientHeight};
            FillRect(window->hdc, &rightRect, blackBrush);
        }
        if (destY > 0) {
            // Top border
            RECT topRect = {0, 0, clientWidth, destY};
            FillRect(window->hdc, &topRect, blackBrush);
            // Bottom border
            RECT bottomRect = {0, destY + destHeight, clientWidth, clientHeight};
            FillRect(window->hdc, &bottomRect, blackBrush);
        }
    }
    
    StretchDIBits(window->hdc,
                 destX, destY, destWidth, destHeight,
                 0, 0, window->surface->width, window->surface->height,
                 window->surface->pixels,
                 &window->bitmapInfo,
                 DIB_RGB_COLORS,
                 SRCCOPY);
}

PlatformSurface* platformGetWindowSurface(PlatformWindow* window) {
    return window ? window->surface : NULL;
}

// Surface management (same as other platforms)
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
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
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
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    
    uint64_t elapsed = now.QuadPart - startTime.QuadPart;
    return (uint32)((elapsed * 1000) / performanceFreq.QuadPart);
}

void platformDelay(uint32 ms) {
    Sleep(ms);
}

// Audio (using waveOut API)
static HWAVEOUT hWaveOut = NULL;
static WAVEHDR waveHeaders[2];
static uint8* audioBuffers[2];
static PlatformAudioCallback audioCallback = NULL;
static void* audioUserData = NULL;
static int audioBufferSize = 0;
static HANDLE audioEvent = NULL;
static HANDLE audioThread = NULL;
static int audioThreadRunning = 0;

DWORD WINAPI AudioThreadProc(LPVOID lpParameter) {
    while (audioThreadRunning) {
        WaitForSingleObject(audioEvent, INFINITE);
        
        if (!audioThreadRunning) break;
        
        // Find a buffer that's done playing
        for (int i = 0; i < 2; i++) {
            if (waveHeaders[i].dwFlags & WHDR_DONE) {
                // Fill buffer with new audio data
                if (audioCallback) {
                    audioCallback(audioUserData, audioBuffers[i], audioBufferSize);
                } else {
                    memset(audioBuffers[i], 128, audioBufferSize); // Silence (128 for 8-bit unsigned)
                }
                
                // Prepare and queue the buffer
                waveOutUnprepareHeader(hWaveOut, &waveHeaders[i], sizeof(WAVEHDR));
                waveHeaders[i].lpData = (LPSTR)audioBuffers[i];
                waveHeaders[i].dwBufferLength = audioBufferSize;
                waveHeaders[i].dwFlags = 0;
                waveOutPrepareHeader(hWaveOut, &waveHeaders[i], sizeof(WAVEHDR));
                waveOutWrite(hWaveOut, &waveHeaders[i], sizeof(WAVEHDR));
            }
        }
    }
    return 0;
}

void CALLBACK waveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance,
                          DWORD_PTR dwParam1, DWORD_PTR dwParam2) {
    if (uMsg == WOM_DONE) {
        SetEvent(audioEvent);
    }
}

int platformInitAudio(void) {
    return 0;
}

void platformCloseAudio(void) {
    if (audioThreadRunning) {
        audioThreadRunning = 0;
        SetEvent(audioEvent);
        WaitForSingleObject(audioThread, INFINITE);
        CloseHandle(audioThread);
        audioThread = NULL;
    }
    
    if (hWaveOut) {
        waveOutReset(hWaveOut);
        
        for (int i = 0; i < 2; i++) {
            if (waveHeaders[i].dwFlags & WHDR_PREPARED) {
                waveOutUnprepareHeader(hWaveOut, &waveHeaders[i], sizeof(WAVEHDR));
            }
            if (audioBuffers[i]) {
                free(audioBuffers[i]);
                audioBuffers[i] = NULL;
            }
        }
        
        waveOutClose(hWaveOut);
        hWaveOut = NULL;
    }
    
    if (audioEvent) {
        CloseHandle(audioEvent);
        audioEvent = NULL;
    }
}

int platformOpenAudio(PlatformAudioSpec* spec) {
    WAVEFORMATEX wfx;
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = spec->channels;
    wfx.nSamplesPerSec = spec->freq;
    wfx.wBitsPerSample = 8;
    wfx.nBlockAlign = wfx.nChannels * wfx.wBitsPerSample / 8;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
    wfx.cbSize = 0;
    
    MMRESULT result = waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx,
                                   (DWORD_PTR)waveOutProc, 0, CALLBACK_FUNCTION);
    if (result != MMSYSERR_NOERROR) {
        lastError = "Failed to open audio device";
        return -1;
    }
    
    audioCallback = spec->callback;
    audioUserData = spec->userdata;
    audioBufferSize = spec->samples * spec->channels;
    
    // Create audio event
    audioEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    
    // Allocate and prepare buffers
    for (int i = 0; i < 2; i++) {
        audioBuffers[i] = (uint8*)malloc(audioBufferSize);
        memset(audioBuffers[i], 128, audioBufferSize); // Silence
        
        memset(&waveHeaders[i], 0, sizeof(WAVEHDR));
        waveHeaders[i].lpData = (LPSTR)audioBuffers[i];
        waveHeaders[i].dwBufferLength = audioBufferSize;
        waveHeaders[i].dwFlags = 0;
        
        waveOutPrepareHeader(hWaveOut, &waveHeaders[i], sizeof(WAVEHDR));
        waveOutWrite(hWaveOut, &waveHeaders[i], sizeof(WAVEHDR));
    }
    
    // Start audio thread
    audioThreadRunning = 1;
    audioThread = CreateThread(NULL, 0, AudioThreadProc, NULL, 0, NULL);
    
    return audioThread ? 0 : -1;
}

void platformPauseAudio(int pause) {
    if (hWaveOut) {
        if (pause) {
            waveOutPause(hWaveOut);
        } else {
            waveOutRestart(hWaveOut);
        }
    }
}

void platformLockAudio(void) {
    // Not needed for waveOut
}

void platformUnlockAudio(void) {
    // Not needed for waveOut
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

#endif // PLATFORM_WINDOWS
