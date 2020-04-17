#ifndef BOARD_H
#define BOARD_H

#include <vector>
#include <limits>
#include <random>
#include <iostream>
#include <graphic.h>

#include <QSharedPointer>

enum {
    None = std::numeric_limits<unsigned>::max(),
};

constexpr auto kStatPeriod = 100;
constexpr auto KStatAlive = 0;
constexpr auto KStatNum = 1;

template <class Bot, size_t W = 256, size_t H = 256>
class Board
{
public:
    Board() : data(W * H, None) {
        for (unsigned i = 0; i != H * W / 16; ++i) {
            unsigned x = rand();
            unsigned y = rand();
            x %= W;
            y %= H;
            if (get(x, y) != None) {
                continue;
            }
            get(x, y) = bots.size();
            bots.push_back(QSharedPointer<Bot>::create(x, y));
            bots.back()->way = abs(rand()) % 4;
        }
        stat_datas.emplace_back("Alive bots");
        for (unsigned i = 0; i != Bot::ComandsNum; ++i) {
            stat_datas.emplace_back(Bot::ComandNames[i] + (" (" + std::to_string(i) + ")"));
            stat_datas.back().push(0);
        }
    }

    unsigned& operator()(size_t i, size_t j) {
        return get(i, j);
    }

    unsigned& get(size_t i, size_t j) {
        return data[i * W + j];
    }

    auto getW() {
        return W;
    }

    auto getH() {
        return H;
    }

    void step() {
        size_t num_of_not_alive = 0;
        auto bs = bots.size();
        for (size_t i = 0; i < bs; ++i) {
            bots[i]->Step(*this);
            if (!bots[i]->isAlive()) {
                ++num_of_not_alive;
            }
        }
        if (num_of_not_alive * 2 > bots.size()) {
            comperess();
        }
        if (steps % kStatPeriod == 0) {
            stat_datas[KStatAlive].push(bots.size() - num_of_not_alive);
            for (unsigned i = 0; i != Bot::ComandsNum; ++i) {
                stat_datas[KStatNum + i].push(Bot::getStat(i) / kStatPeriod);
            }
        }
        ++steps;
    }

    void stat() {
        comperess();
        std::cerr << bots.size() << std::endl;
        double sum = 0, sage = 0;
        std::vector<unsigned> a(Bot::ComandsNum);
        for (auto && bot : bots) {
            sum += bot->energy;
            sage += bot->age;
            for (auto g : bot->genome) {
                ++a[g % a.size()];
            }
        }
        std::cerr << sum / bots.size() << std::endl;
        std::cerr << sage / bots.size() << std::endl;
        std::cerr << steps << std::endl;
        for (auto elem : a) {
            std::cerr << elem << ' ';
        }
        std::cerr << std::endl;
    }

    void comperess() {
        size_t to_write = 0;
        std::vector<unsigned> ids(bots.size(), None);
        for (size_t i = 0; i != bots.size(); ++i) {
            if (bots[i]->x == -1 && bots[i]->y == -1) {
                continue;
            } else if (i != to_write) {
                ids[i] = to_write;
                bots[to_write] = std::move(bots[i]);
                ++to_write;
            } else {
                ids[i] = to_write;
                ++to_write;
            }
        }
        for (auto && elem : data) {
            if (elem != None) {
                elem = ids[elem];
            }
        }
        bots.resize(to_write);
    }

    unsigned& get(Bot &bot, int dx, int dy) {
        unsigned new_x = (bot.x + dx) % H;
        unsigned new_y = (bot.y + dy) % W;
        return get(new_x, new_y);
    }

    void check_and_go(Bot &bot, int dx, int dy) {
        unsigned new_x = (bot.x + dx) % H;
        unsigned new_y = (bot.y + dy) % W;
        if (get(new_x, new_y) != None) {
            return;
        }
        get(new_x, new_y) = get(bot.x, bot.y);
        get(bot.x, bot.y) = None;
        bot.x = new_x;
        bot.y = new_y;
    }

    void new_bot(const Bot &bot, int dx, int dy, unsigned char acc) {
        unsigned new_x = (bot.x + dx) % H;
        unsigned new_y = (bot.y + dy) % W;
        if (get(new_x, new_y) != None) {
            return;
        }
        get(new_x, new_y) = bots.size();
        bots.back()->type = bot.type;
        bots.push_back(QSharedPointer<Bot>::create(bot, new_x, new_y, acc));
    }

    void new_bot(const Bot &bot1, const Bot &bot2, int dx, int dy, unsigned char acc) {
        unsigned new_x = (bot1.x + dx) % H;
        unsigned new_y = (bot1.y + dy) % W;
        if (get(new_x, new_y) != None) {
            return;
        }
        get(new_x, new_y) = bots.size();
        bots.back()->type = bot1.type;
        bots.push_back(QSharedPointer<Bot>::create(bot1, bot2, new_x, new_y, acc));
    }

    auto& getBot(unsigned bot_id) {
        return bots[bot_id];
    }

    QSharedPointer<Bot> getBotUnsave(unsigned bot_id) {
        if (bot_id > bots.size()) {
            return {};
        }
        return bots[bot_id];
    }

    static unsigned dist(int a, int b, int x, int y) {
        return abs(a - x) + abs(b - y);
    }

    unsigned syntes(Bot &bot) {
        unsigned state = dist(2 * H / 5, W / 2, bot.x, bot.y);
        if (!resource_zone || state > H / 8) {
            return abs(rand()) % 3;
        } else {
            return std::min(3 * (H / 8 - state), 30ul);
        }
    }

    unsigned minerals(Bot &bot) {
        unsigned state = dist(3 * H / 5, W / 2, bot.x, bot.y);
        if (!resource_zone || state > H / 8) {
            return abs(rand()) % 2;
        } else {
            return 10;
        }
    }

public:
    /// Render settings
    unsigned rendertype = 0;
    unsigned netrender = 0;
    bool wayrender = false;

    /// Other settings
    bool resource_zone = true;

    /// Stats
    std::vector<Graphic::Data> stat_datas;

private:
    std::vector<unsigned> data;
    std::vector<QSharedPointer<Bot>> bots;
    unsigned steps = 0;
};

#endif // BOARD_H
