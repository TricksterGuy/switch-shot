#ifndef PUZZLE_HPP
#define PUZZLE_HPP

#include <cstdint>
#include <unordered_set>
#include <vector>

struct pair_hash {

    size_t operator()(const std::pair<uint32_t, uint32_t>& pair) const
    {
        return static_cast<size_t>(pair.first) << 32 | pair.second;
    }
};

class Puzzle
{
public:
    typedef std::unordered_set<std::pair<uint32_t, uint32_t>, pair_hash> point_set;
    static constexpr uint8_t EMPTY = 255;

    Puzzle(uint32_t w, uint32_t h, uint8_t c = 5) : width(w), height(h), colors(c), data(w * h, EMPTY)
    {
        randomize();
    }
    uint8_t at(uint32_t x, uint32_t y) const {return data[y * width + x];}
    uint32_t match(uint32_t x, uint32_t y);
    point_set test(uint32_t x, uint32_t y) const;

    void randomize();
    void compact(const point_set& hints);

    uint32_t width;
    uint32_t height;
    uint8_t colors;
    std::vector<uint8_t> data;

};


#endif
