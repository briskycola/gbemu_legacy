#include "../include/ui.h"
#include "../include/bus.h"
#include "../include/emulator.h"
#include "../include/gamepad.h"
#include "../include/ppu.h"

#include <SDL2/SDL.h>

SDL_Window *sdlWindow;
SDL_Renderer *sdlRenderer;
SDL_Texture *sdlTexture;
SDL_Surface *screen;

SDL_Window *sdlDebugWindow;
SDL_Renderer *sdlDebugRenderer;
SDL_Texture *sdlDebugTexture;
SDL_Surface *debugScreen;

static int scale = 4;

void uiInit()
{

    SDL_Init(SDL_INIT_VIDEO);

    SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0, &sdlWindow, &sdlRenderer);

    screen = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32,
                                  0x00FF0000,
                                  0x0000FF00,
                                  0x000000FF,
                                  0xFF000000);
    sdlTexture = SDL_CreateTexture(sdlRenderer,
                                   SDL_PIXELFORMAT_ARGB8888,
                                   SDL_TEXTUREACCESS_STREAMING,
                                   SCREEN_WIDTH, SCREEN_HEIGHT);

    debugScreen = SDL_CreateRGBSurface(0, (16 * 8 * scale) + (16 * scale),
                                       (32 * 8 * scale) + (64 * scale), 32,
                                       0x00FF0000,
                                       0x0000FF00,
                                       0x000000FF,
                                       0xFF000000);

    sdlDebugTexture = SDL_CreateTexture(sdlDebugRenderer,
                                        SDL_PIXELFORMAT_ARGB8888,
                                        SDL_TEXTUREACCESS_STREAMING,
                                        (16 * 8 * scale) + (16 * scale),
                                        (32 * 8 * scale) + (64 * scale));

    int x, y;
    SDL_GetWindowPosition(sdlWindow, &x, &y);
    SDL_SetWindowPosition(sdlDebugWindow, x + SCREEN_WIDTH + 10, y);
}

void delay(uint32_t ms)
{
    SDL_Delay(ms);
}

uint32_t getTicks()
{
    return SDL_GetTicks();
}

static unsigned long tileColors[4] = {0xFFFFFFFF, 0xFFAAAAAA, 0xFF555555, 0xFF000000};

void displayTile(SDL_Surface *surface, uint16_t startLocation, uint16_t tileNum, int x, int y)
{
    SDL_Rect rc;

    for (int tileY = 0; tileY < 16; tileY += 2)
    {
        uint8_t b1 = busRead(startLocation + (tileNum * 16) + tileY);
        uint8_t b2 = busRead(startLocation + (tileNum * 16) + tileY + 1);

        for (int bit = 7; bit >= 0; bit--)
        {
            uint8_t hi = !!(b1 & (1 << bit)) << 1;
            uint8_t lo = !!(b2 & (1 << bit));

            uint8_t color = hi | lo;

            rc.x = x + ((7 - bit) * scale);
            rc.y = y + (tileY / 2 * scale);
            rc.w = scale;
            rc.h = scale;

            SDL_FillRect(surface, &rc, tileColors[color]);
        }
    }
}

void uiUpdate()
{
    SDL_Rect rc;
    rc.x = rc.y = 0;
    rc.w = rc.h = 2048;

    uint32_t *videoBuffer = ppuGetContext()->videoBuffer;

    for (int lineNum = 0; lineNum < YRES; lineNum++)
    {
        for (int x = 0; x < XRES; x++)
        {
            rc.x = x * scale;
            rc.y = lineNum * scale;
            rc.w = scale;
            rc.h = scale;

            SDL_FillRect(screen, &rc, videoBuffer[x + (lineNum * XRES)]);
        }
    }

    SDL_UpdateTexture(sdlTexture, NULL, screen->pixels, screen->pitch);
    SDL_RenderClear(sdlRenderer);
    SDL_RenderCopy(sdlRenderer, sdlTexture, NULL, NULL);
    SDL_RenderPresent(sdlRenderer);
}

void uiOnKey(bool down, uint32_t keyCode)
{

    switch (keyCode)
    {
    case SDLK_p:
        gamepadGetState()->b = down;
        break;
    case SDLK_o:
        gamepadGetState()->a = down;
        break;
    case SDLK_RETURN:
        gamepadGetState()->start = down;
        break;
    case SDLK_TAB:
        gamepadGetState()->select = down;
        break;
    case SDLK_w:
        gamepadGetState()->up = down;
        break;
    case SDLK_s:
        gamepadGetState()->down = down;
        break;
    case SDLK_a:
        gamepadGetState()->left = down;
        break;
    case SDLK_d:
        gamepadGetState()->right = down;
        break;
    }
}

void uiHandleEvents()
{
    SDL_Event e;
    while (SDL_PollEvent(&e) > 0)
    {
        if (e.type == SDL_KEYDOWN)
        {
            uiOnKey(true, e.key.keysym.sym);
        }

        if (e.type == SDL_KEYUP)
        {
            uiOnKey(false, e.key.keysym.sym);
        }

        if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_CLOSE)
        {
            emuGetContext()->die = true;
        }
    }
}
