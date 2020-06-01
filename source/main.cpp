#include <memory>

#include <switch.h>
#include "SDLGame.hpp"
#include "NFont.h"
#include "SDL_FontCache.h"

#include "puzzle.hpp"
#include "color_modulation.hpp"

constexpr uint32_t GAME_WIDTH = SCREEN_WIDTH;
constexpr uint32_t GAME_HEIGHT = SCREEN_HEIGHT - 120;

class SwitchShot : public SDLGame
{
public:
    SwitchShot() : SDLGame("SwitchShot!") {}

    bool Initialize() override;
    void New(time_t seeded_game = 0) override;
    void Update() override;
    void Draw() override;
    void Destroy() override;

private:
    void OnTouchMotion(const SDL_TouchFingerEvent& event) override;
    void OnTouchDown(const SDL_TouchFingerEvent& event) override;
    void OnButtonDown(const SDL_JoyButtonEvent& event) override;

    std::pair<uint32_t, uint32_t> GetCoords(float x, float y) const;
    void DoMatch(uint32_t tile_x, uint32_t tile_y);
    void DoSelectSet(uint32_t tile_x, uint32_t tile_y);

    SDL_Texture* cursor = nullptr;
    std::unique_ptr<NFont> font;

    std::unique_ptr<Puzzle> puzzle;
    uint32_t score;

    std::vector<std::tuple<uint8_t, uint8_t, uint8_t>> colors;
    std::pair<uint32_t, uint32_t> current_tile;
    Puzzle::point_set points;
    ColorModulation modulation;
};

bool SwitchShot::Initialize()
{
    if (!SDLGame::Initialize())
        return false;

    romfsInit();

    SDL_Surface* surface = IMG_Load("romfs:/graphics/cursor.png");
    if (!surface)
    {
         printf("IMG_Load: %s\n", IMG_GetError());
         return false;
    }

    cursor = SDL_CreateTextureFromSurface(renderer, surface);
    if (!cursor)
    {
        printf("CreateTextureFromSurface failed: %s\n", SDL_GetError());
        SDL_FreeSurface(surface);
        return false;
    }
    SDL_FreeSurface(surface);

    font.reset(new NFont(renderer, "romfs:/fonts/FreeSans.ttf", 60));

    New();

    return true;
}

void SwitchShot::New(time_t seeded_game)
{
    SDLGame::New(seeded_game);

    colors.clear();
    for (int i = 0; i < 4; i++)
        colors.push_back({randomInt(48, 255 - 48), randomInt(48, 255 - 48), randomInt(48, 255 - 48)});

    puzzle.reset(new Puzzle(16, 8, 4));

    current_tile = {puzzle->width / 2, puzzle->height / 2};

    score = 0;
}

void SwitchShot::Update()
{
    modulation.update();
}

void SwitchShot::Draw()
{
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

            SDL_Rect rect = {static_cast<int>(x) * 120 + 1, static_cast<int>(y) * 120 + 1, 118, 118};
            SDL_RenderFillRect(renderer, &rect);
        }
    }

    if (current_tile != std::make_pair(-1U, -1U))
    {
        SDL_Rect rect = {static_cast<int>(current_tile.first) * 120, static_cast<int>(current_tile.second) * 120, 120, 120};
        SDL_RenderCopy(renderer, cursor, nullptr, &rect);
    }

    font->draw(renderer, 0, 8 * 120, NFont::Color(128, 128, 255), "Score: %d", score);
}

void SwitchShot::Destroy()
{
    if (cursor) SDL_DestroyTexture(cursor);
    cursor = nullptr;
    SDLGame::Destroy();
    romfsExit();
}

std::pair<uint32_t, uint32_t> SwitchShot::GetCoords(float x, float y) const
{
    uint32_t sx = static_cast<uint32_t>(x * SCREEN_WIDTH);
    uint32_t sy = static_cast<uint32_t>(y * SCREEN_HEIGHT);

    if (sx > GAME_WIDTH || sy > GAME_HEIGHT)
        return {-1, -1};

    return {std::max(std::min(sx / 120, puzzle->width - 1), 0U),
            std::max(std::min(sy / 120, puzzle->height - 1), 0U)};
}

void SwitchShot::OnTouchDown(const SDL_TouchFingerEvent& event)
{
    auto [tile_x, tile_y] = GetCoords(event.x, event.y);
    DoMatch(tile_x, tile_y);
}

void SwitchShot::OnTouchMotion(const SDL_TouchFingerEvent& event)
{
    auto [tile_x, tile_y] = GetCoords(event.x, event.y);
    DoSelectSet(tile_x, tile_y);
}

void SwitchShot::OnButtonDown(const SDL_JoyButtonEvent& event)
{
    switch (event.button)
    {
        case SDL_KEY_MINUS:
            New();
            break;
        case SDL_KEY_DRIGHT:
        case SDL_KEY_LSTICK_RIGHT:
        case SDL_KEY_RSTICK_RIGHT:
            current_tile.first = std::min(current_tile.first + 1, puzzle->width - 1);
            DoSelectSet(current_tile.first, current_tile.second);
            break;
        case SDL_KEY_DLEFT:
        case SDL_KEY_LSTICK_LEFT:
        case SDL_KEY_RSTICK_LEFT:
            current_tile.first = std::max(current_tile.first - 1, 0U);
            DoSelectSet(current_tile.first, current_tile.second);
            break;
        case SDL_KEY_DDOWN:
        case SDL_KEY_LSTICK_DOWN:
        case SDL_KEY_RSTICK_DOWN:
            current_tile.second = std::min(current_tile.second + 1, puzzle->height - 1);
            DoSelectSet(current_tile.first, current_tile.second);
            break;
        case SDL_KEY_DUP:
        case SDL_KEY_LSTICK_UP:
        case SDL_KEY_RSTICK_UP:
            current_tile.second = std::max(current_tile.second - 1, 0U);
            DoSelectSet(current_tile.first, current_tile.second);
            break;
        case SDL_KEY_A:
        case SDL_KEY_B:
            DoMatch(current_tile.first, current_tile.second);
            break;
        case SDL_KEY_X:
            New(seed);
            break;
        default:
            break;
    }
}

void SwitchShot::DoSelectSet(uint32_t tile_x, uint32_t tile_y)
{
    if (tile_x == -1U || tile_y == -1U)
    {
        points.clear();
        return;
    }

    if (puzzle->at(tile_x, tile_y) == Puzzle::EMPTY)
        return;

    current_tile = {tile_x, tile_y};
    if (points.find({tile_x, tile_y}) == points.end())
    {
        points = puzzle->test(tile_x, tile_y);
        uint8_t current_color = puzzle->at(tile_x, tile_y);
        current_tile = {tile_x, tile_y};
        if (current_color != Puzzle::EMPTY)
        {
            auto [r, g, b] = colors[current_color];
            modulation.set(RGBA8_MAXALPHA(std::max(0,   r - 48), std::max(0,   g - 48), std::max(0,   b - 48)),
                           RGBA8_MAXALPHA(std::min(255, r + 48), std::min(255, g + 48), std::min(255, b + 48)), 60);
        }
    }
}

void SwitchShot::DoMatch(uint32_t tile_x, uint32_t tile_y)
{
    if (tile_x == -1U || tile_y == -1U)
    {
        points.clear();
        return;
    }

    if (puzzle->at(tile_x, tile_y) == Puzzle::EMPTY)
        return;

    if (points.find({tile_x, tile_y}) == points.end())
    {
        DoSelectSet(tile_x, tile_y);
        return;
    }

    uint32_t matches = puzzle->match(tile_x, tile_y) - 1;
    score += matches * matches;

    points.clear();
}

int main(int argc, char *argv[])
{
    SwitchShot game;
    if (game.Initialize())
        game.Run();
    game.Destroy();
    return 0;
}
