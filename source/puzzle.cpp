#include "puzzle.hpp"

#include <cstdlib>
#include <deque>
#include <list>
#include <set>

uint32_t Puzzle::match(uint32_t x, uint32_t y)
{
    auto matched = test(x, y);

    if (matched.size() <= 1)
        return 1;

    for (const auto [x, y] : matched)
        data[y * width + x] = EMPTY;

    compact(matched);

    return matched.size();
}

Puzzle::point_set Puzzle::test(uint32_t x, uint32_t y) const
{
    point_set visited;

    uint8_t color = data[y * width + x];
    if (color == EMPTY)
        return visited;

    std::deque<std::pair<uint32_t, uint32_t>> queue;

    queue.push_back({x, y});
    visited.insert({x, y});

    while (!queue.empty())
    {
        auto [x, y] = queue.front();
        queue.pop_front();

        if (visited.find({x - 1, y}) == visited.end() && x >= 1         && data[y * width + x - 1] == color)
        {
            queue.push_back({x - 1, y});
            visited.insert({x - 1, y});
        }
        if (visited.find({x + 1, y}) == visited.end() && x + 1 < width  && data[y * width + x + 1] == color)
        {
            queue.push_back({x + 1, y});
            visited.insert({x + 1, y});
        }
        if (visited.find({x, y - 1}) == visited.end() && y >= 1         && data[(y - 1) * width + x] == color)
        {
            queue.push_back({x, y - 1});
            visited.insert({x, y - 1});
        }
        if (visited.find({x, y + 1}) == visited.end() && y + 1 < height && data[(y + 1) * width + x] == color)
        {
            queue.push_back({x, y + 1});
            visited.insert({x, y + 1});
        }
    }

    if (visited.size() == 1)
        visited.clear();

    return visited;
}

void Puzzle::compact(const point_set& hints)
{
    uint32_t minx = width, miny = height, maxx = 0, maxy = 0;
    for (const auto [x, y] : hints)
    {
        minx = std::min(x, minx);
        miny = std::min(y, miny);
        maxx = std::max(x, maxx);
        maxy = std::max(y, maxy);
    }

    int32_t x_mark = -1;
    for (uint32_t x = minx; x <= maxx; x++)
    {
        int32_t y_mark = -1;
        bool moved_one = false;
        for (int32_t y = maxy; y >= 0 ; y--)
        {
            if (y_mark == -1 && data[y * width + x] == EMPTY)
                y_mark = y;
            else if ((y_mark != -1 || x_mark != -1) && data[y * width + x] != EMPTY)
            {
                int32_t movex = x_mark == -1 ? x : x_mark;
                int32_t movey = y_mark == -1 ? y : y_mark;
                std::swap(data[movey * width + movex], data[y * width + x]);
                if (y_mark != -1) y_mark--;
                moved_one = true;
            }
        }
        if (x_mark == -1 && maxy == height - 1 && data[(height - 1) * width + x] == EMPTY)
        {
            x_mark = x;
            maxx = width - 1;
        }
        else if (x_mark != -1 && (data[(height - 1) * width + x] != EMPTY || moved_one))
            x_mark++;
    }
}

void Puzzle::randomize()
{
    for (unsigned int i = 0; i < data.size(); i++)
        data[i] = rand() % colors;
}
