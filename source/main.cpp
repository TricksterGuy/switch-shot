#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <memory>

#include <switch.h>
#include <SDL.h>
#include "NFont.h"
#include "SDL_FontCache.h"

#include "puzzle.hpp"
#include "color_modulation.hpp"

class Game
{
public:
    bool Initialize();
    void New();
    void Run();
    bool Update();
    bool Input();
    void Draw();
    void Destroy();
private:
    void OnTouchMotion(const SDL_TouchFingerEvent& event);
    void OnTouchDown(const SDL_TouchFingerEvent& event);
    void OnTouchUp(const SDL_TouchFingerEvent& event);
    void OnButtonDown(const SDL_JoyButtonEvent& event);
    void OnButtonUp(const SDL_JoyButtonEvent& event);

    SDL_Window* window;
    SDL_Renderer* renderer;
    std::unique_ptr<NFont> font;
    std::unique_ptr<Puzzle> puzzle;
    uint32_t score;
    std::vector<std::tuple<uint8_t, uint8_t, uint8_t>> colors;
    std::pair<uint32_t, uint32_t> current_tile;
    uint8_t current_color;
    Puzzle::point_set points;
    ColorModulation modulation;
};

bool Game::Initialize()
{
    romfsInit();

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0)
    {
        SDL_Log("SDL_Init: %s\n", SDL_GetError());
        return false;
    }

    window = SDL_CreateWindow("??????", 0, 0, 1920, 1080, 0);
    if (!window)
    {
        SDL_Log("SDL_CreateWindow: %s\n", SDL_GetError());
        SDL_Quit();
        return false;
    }

    renderer = SDL_CreateRenderer(window, 0, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer)
    {
        SDL_Log("SDL_CreateRenderer: %s\n", SDL_GetError());
        SDL_Quit();
        return false;
    }

    for (int i = 0; i < 2; i++)
    {
        if (SDL_JoystickOpen(i) == NULL)
        {
            SDL_Log("SDL_JoystickOpen: %s\n", SDL_GetError());
            SDL_Quit();
            return -1;
        }
    }

    // Initialize console. Using NULL as the second argument tells the console library to use the internal console structure as current one.
    //consoleInit(NULL);

    font.reset(new NFont(renderer, "romfs:/fonts/FreeSans.ttf", 60));

    New();

    return true;
}

void Game::New()
{
    srand(time(NULL));

    colors.clear();
    for (int i = 0; i < 4; i++)
        colors.push_back({rand() % 255, rand() % 255, rand() % 255});

    puzzle.reset(new Puzzle(16, 8, 4));

    current_tile = {-1, -1};
    current_color = Puzzle::EMPTY;

    score = 0;
}

void Game::Run()
{
    bool running = true;
    do
    {
        running = Update();
        SDL_RenderPresent(renderer);
    }
    while(running /*&& appletMainLoop()*/);
}

bool Game::Update()
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    if (!Input()) return false;

    modulation.update();

    Draw();

    return true;
}

bool Game::Input()
{
    /*// Scan all the inputs. This should be done once for each frame
    hidScanInput();

    // hidKeysDown returns information about which buttons have been just pressed (and they weren't in the previous frame)
    u64 kDown = hidKeysDown(CONTROLLER_P1_AUTO);
    if (kDown & KEY_PLUS) return false; // break in order to return to hbmenu*/

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_FINGERMOTION:
            OnTouchMotion(event.tfinger);
            break;
        case SDL_FINGERDOWN:
            OnTouchDown(event.tfinger);
            break;
        case SDL_FINGERUP:
            OnTouchUp(event.tfinger);
            break;
        case SDL_JOYAXISMOTION:
            SDL_Log("Joystick %d axis %d value: %d\n",
                    event.jaxis.which,
                    event.jaxis.axis, event.jaxis.value);
            break;
        case SDL_JOYBUTTONDOWN:
            // https://github.com/devkitPro/SDL/blob/switch-sdl2/src/joystick/switch/SDL_sysjoystick.c#L52
            // seek for joystick #0
            if (event.jbutton.which == 0)
            {
                OnButtonDown(event.jbutton);

                if (event.jbutton.button == KEY_PLUS)
                    return false;
            }
            break;

        default:
            break;
        }
    }
    return true;
}

void Game::OnTouchDown(const SDL_TouchFingerEvent& event)
{
    uint32_t x = static_cast<uint32_t>(event.x * 1920);
    uint32_t y = static_cast<uint32_t>(event.y * 1080);

    if (y > 8 * 120) return;

    uint32_t tile_x, tile_y;
    tile_x = std::max(std::min(x / 120, puzzle->width - 1), 0U);
    tile_y = std::max(std::min(y / 120, puzzle->height - 1), 0U);

    if (points.find({tile_x, tile_y}) == points.end())
    {
        OnTouchMotion(event);
        return;
    }

    uint32_t matches = puzzle->match(tile_x, tile_y) - 1;
    score += matches * matches;

    points.clear();
    current_tile = {-1, -1};
}

void Game::OnTouchUp(const SDL_TouchFingerEvent& event)
{

}

void Game::OnTouchMotion(const SDL_TouchFingerEvent& event)
{
    uint32_t x = static_cast<uint32_t>(event.x * 1920);
    uint32_t y = static_cast<uint32_t>(event.y * 1080);

    if (y > 8 * 120) return;

    uint32_t tile_x, tile_y;
    tile_x = std::max(std::min(x / 120, puzzle->width - 1), 0U);
    tile_y = std::max(std::min(y / 120, puzzle->height - 1), 0U);

    if (current_tile != std::make_pair(tile_x, tile_y) && points.find({tile_x, tile_y}) == points.end())
    {
        points = puzzle->test(tile_x, tile_y);
        current_color = puzzle->at(tile_x, tile_y);
        if (current_color != Puzzle::EMPTY)
        {
            auto [r, g, b] = colors[current_color];
            modulation.set(RGBA8_MAXALPHA(std::max(0,   r - 48), std::max(0,   g - 48), std::max(0,   b - 48)),
                           RGBA8_MAXALPHA(std::min(255, r + 48), std::min(255, g + 48), std::min(255, b + 48)), 60);
        }
    }
}

void Game::OnButtonDown(const SDL_JoyButtonEvent& event)
{
    if (event.button == KEY_MINUS)
        New();
}

void Game::OnButtonUp(const SDL_JoyButtonEvent& event)
{

}


void Game::Draw()
{
    // Updates the screen.
    //consoleUpdate(NULL);

    for (uint32_t y = 0; y < puzzle->height; y++)
    {
        for (uint32_t x = 0; x < puzzle->width; x++)
        {
            uint8_t c = puzzle->at(x, y);

            if (c == Puzzle::EMPTY) continue;

            auto [r, g, b] = colors[c];

            if (points.find({x, y}) != points.end())
                SDL_SetRenderDrawColor(renderer, modulation.red(), modulation.green(), modulation.blue(), 255);
            else
                SDL_SetRenderDrawColor(renderer, r, g, b, 255);

            SDL_Rect rect = {static_cast<int>(x) * 120, static_cast<int>(y) * 120, 120, 120};
            SDL_RenderFillRect(renderer, &rect);
        }
    }
    font->draw(renderer, 0, 8 * 120, NFont::Color(128, 128, 255), "Score: %d", score);
}

void Game::Destroy()
{
    romfsExit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    /*consoleExit(NULL); */
}

int main(int argc, char *argv[])
{
    Game game;
    game.Initialize();
    game.Run();
    game.Destroy();
    return 0;
}
