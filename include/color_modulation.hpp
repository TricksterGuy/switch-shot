#ifndef COLOR_MODULATION_HPP
#define COLOR_MODULATION_HPP

#include <cstdint>

class ColorModulation
{
public:
    ColorModulation() {}
    ColorModulation(uint32_t min, uint32_t max, int speed);
    void set(uint32_t min, uint32_t max, int speed);
    uint8_t red() const {return r;}
    uint8_t green() const {return g;}
    uint8_t blue() const {return b;}
    uint8_t alpha() const {return a;}
    void update();
private:
    uint8_t r, g, b, a;
    uint8_t minr, ming, minb, mina;
    uint8_t maxr, maxg, maxb, maxa;
    int32_t speed;
    int32_t count = 0;
};

#endif
