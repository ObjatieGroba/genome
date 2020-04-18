#include "board.h"

static thread_local unsigned long xrrr=123456789, yrrr=362436069, zrrr=521288629;

unsigned long xorshf96() {
    unsigned long t;
    xrrr ^= xrrr << 16u;
    xrrr ^= xrrr >> 5u;
    xrrr ^= xrrr << 1u;

    t = xrrr;
    xrrr = yrrr;
    yrrr = zrrr;
    zrrr = t ^ xrrr ^ yrrr;

    return zrrr;
}
