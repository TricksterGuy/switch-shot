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


constexpr uint32_t SCREEN_WIDTH = 1920;
constexpr uint32_t SCREEN_HEIGHT = 1080;

constexpr uint32_t GAME_WIDTH = SCREEN_WIDTH;
constexpr uint32_t GAME_HEIGHT = SCREEN_HEIGHT - 120;


enum SDLKeyMapping {
        SDL_KEY_A, SDL_KEY_B, SDL_KEY_X, SDL_KEY_Y,
        SDL_KEY_LSTICK, SDL_KEY_RSTICK,
        SDL_KEY_L, SDL_KEY_R,
        SDL_KEY_ZL, SDL_KEY_ZR,
        SDL_KEY_PLUS, SDL_KEY_MINUS,
        SDL_KEY_DLEFT, SDL_KEY_DUP, SDL_KEY_DRIGHT, SDL_KEY_DDOWN,
        SDL_KEY_LSTICK_LEFT, SDL_KEY_LSTICK_UP, SDL_KEY_LSTICK_RIGHT, SDL_KEY_LSTICK_DOWN,
        SDL_KEY_RSTICK_LEFT, SDL_KEY_RSTICK_UP, SDL_KEY_RSTICK_RIGHT, SDL_KEY_RSTICK_DOWN,
        SDL_KEY_SL_LEFT, SDL_KEY_SR_LEFT, SDL_KEY_SL_RIGHT, SDL_KEY_SR_RIGHT
};

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

    std::pair<uint32_t, uint32_t> GetCoords(float x, float y) const;
    void DoSelectSet(uint32_t tile_x, uint32_t tile_y);

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

    window = SDL_CreateWindow("SwitchShot!", 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
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
    if (kDown & SDL_KEY_PLUS) return false; // break in order to return to hbmenu*/

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
            case SDL_JOYBUTTONDOWN:
                if (event.jbutton.which == 0)
                {
                    OnButtonDown(event.jbutton);

                    if (event.jbutton.button == SDL_KEY_PLUS)
                        return false;
                }
                break;

            default:
                break;
        }
    }
    return true;
}

std::pair<uint32_t, uint32_t> Game::GetCoords(float x, float y) const
{
    uint32_t sx = static_cast<uint32_t>(x * SCREEN_WIDTH);
    uint32_t sy = static_cast<uint32_t>(y * SCREEN_HEIGHT);

    if (sx > GAME_WIDTH || sy > GAME_HEIGHT)
        return {-1, -1};

    return {std::max(std::min(sx / 120, puzzle->width - 1), 0U),
            std::max(std::min(sy / 120, puzzle->height - 1), 0U)};
}

void Game::OnTouchDown(const SDL_TouchFingerEvent& event)
{
    auto [tile_x, tile_y] = GetCoords(event.x, event.y);

    if (points.find({tile_x, tile_y}) == points.end())
    {
        DoSelectSet(tile_x, tile_y);
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
    auto [tile_x, tile_y] = GetCoords(event.x, event.y);
    DoSelectSet(tile_x, tile_y);
}

void Game::OnButtonDown(const SDL_JoyButtonEvent& event)
{
    if (event.button == SDL_KEY_MINUS)
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

void Game::DoSelectSet(uint32_t tile_x, uint32_t tile_y)
{
    if (tile_x == -1 || tile_y == -1)
        return;

    if (points.find({tile_x, tile_y}) == points.end())
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

int main(int argc, char *argv[])
{
    Game game;
    game.Initialize();
    game.Run();
    game.Destroy();
    return 0;
}
