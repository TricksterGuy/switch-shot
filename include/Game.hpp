#ifndef GAME_HPP
#define GAME_HPP

#include <cstdlib>
#include <ctime>

inline int randomInt(int max)
{
    return rand() / (RAND_MAX / max + 1);
}

inline int randomInt(int start, int end)
{
    return randomInt(end - start + 1) + start;
}

class Game
{
public:
    Game() {}
    virtual ~Game() {Destroy();}
    virtual bool Initialize();
    virtual void New(time_t seeded_game = 0)
    {
        if (seeded_game == 0)
            seeded_game = time(NULL);
        srand(seed = seeded_game);
    }
    virtual void Run()
    {
        while(true)
        {
            if (!Input())
                break;
            Update();
            Draw();
        }
    }
    virtual void Update() = 0;
    virtual bool Input() {return true;}
    virtual void Draw() {}
    virtual void Destroy() {}
protected:
    time_t seed = 0;

};

#endif
