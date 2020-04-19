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

thread_local unsigned thread_id = 0;

unsigned get_thread_id() {
    return thread_id;
}

void set_thread_id(unsigned id) {
    thread_id = id;
}

namespace Stat {
    std::vector<unsigned short> data(kStatNum, 0);
    unsigned threads_num = 0;
    unsigned stat_num = 0;
}

void Stat::init(unsigned num) {
    stat_num += num;
    data.resize(stat_num * threads_num);
    std::fill(data.begin(), data.end(), 0);
}

void Stat::add_threads(unsigned num) {
    threads_num += num;
    data.resize(stat_num * threads_num);
    std::fill(data.begin(), data.end(), 0);
}

void Stat::inc(unsigned id) {
    ++data[get_thread_id() * stat_num + id];
}

void Stat::inc(unsigned id, unsigned add) {
    data[get_thread_id() * stat_num + id] += add;
}

unsigned Stat::get(unsigned id) {
    unsigned res = 0;
    for (auto i = id; i < data.size(); i += stat_num) {
        res += data[i];
        data[i] = 0;
    }
    return res;
}
