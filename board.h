#ifndef BOARD_H
#define BOARD_H

#include <vector>
#include <limits>
#include <iostream>
#include <mutex>
#include <graphic.h>

#include <QSharedPointer>
#include <condition_variable>
#include <thread>

enum {
    None = std::numeric_limits<unsigned>::max(),
};

unsigned long xorshf96();

constexpr auto kStatPeriod = 100;
constexpr auto kStatAlive = 0;
constexpr auto kStatBPS = 1;
constexpr auto kStatNum = 2;

template <class Bot, size_t BoardWidth = 256, size_t MutexLockWidth = 16>
class Board : public std::enable_shared_from_this<Board<Bot, BoardWidth, MutexLockWidth>>
{
public:
    Board() : data(BoardWidth * BoardWidth, None) {
        static_assert(BoardWidth % MutexLockWidth == 0);
        for (unsigned i = 0; i != BoardWidth * BoardWidth / 16; ++i) {
            unsigned x = xorshf96();
            unsigned y = xorshf96();
            x %= BoardWidth;
            y %= BoardWidth;
            if (get(x, y) != None) {
                continue;
            }
            get(x, y) = bots.size();
            bots.push_back(QSharedPointer<Bot>::create(x, y));
            bots.back()->way = xorshf96() % 4;
        }
        stat_data.emplace_back("Alive bots");
        stat_data.emplace_back("Bots per second");
        for (unsigned i = 0; i != Bot::ComandsNum; ++i) {
            stat_data.emplace_back(Bot::ComandNames[i] + (" (" + std::to_string(i) + ")"));
            stat_data.back().push(0);
        }
    }

    ~Board() {
        killed = true;
        std::unique_lock lock(main_mx);
        running_threads += threads.size();
        cv_start.notify_all();
        while (killed) {
            cv_finish.wait(lock);
        }
        for (auto && thread : threads) {
            thread.join();
        }
    }

    void run_threads(size_t num) {
        if (num > BoardWidth / MutexLockWidth / 2) {
            std::cerr << "Too many threads";
        }
        std::unique_lock main_lock(main_mx);
        auto self = this->shared_from_this();
        while (num-- > 0) {
            new_bots.emplace_back();
            threads.emplace_back([self, thread_id=threads.size()]() {
                std::unique_lock main_lock(self->main_mx);
                while (true) {
                    while (!self->killed && self->tasks.empty()) {
                        self->cv_start.wait(main_lock);
                    }
                    if (self->killed) {
                        break;
                    }
                    size_t start_width = self->tasks.back();
                    self->tasks.pop_back();
                    ++self->running_threads;
                    main_lock.unlock();
                    {
                        size_t start_height = 0;
                        size_t wmutex = start_width / MutexLockWidth;
                        size_t wmutex2 = wmutex + 1;
                        if (wmutex2 == self->mxs.size()) {
                            wmutex2 = 0;
                        }
                        std::unique_lock lock1(self->mxs[wmutex]);
                        std::unique_lock lock2(self->mxs[wmutex2]);
                        while (start_height < BoardWidth) {
                            for (size_t i = 0; i != MutexLockWidth; ++i) {
                                for (size_t j = 0; j != MutexLockWidth; ++j) {
                                    auto id = self->get(start_width + i, start_height + j);
                                    if (id != None) {
                                        self->bots[id]->Step(*self, thread_id);
                                    }
                                }
                            }
                            start_height += MutexLockWidth;
                        }
                    }
                    main_lock.lock();
                    if (--self->running_threads == 0) {
                        self->cv_finish.notify_one();
                    }
                }
                if (--self->running_threads == 0) {
                    self->killed = false;
                    self->cv_finish.notify_one();
                }
            });
        }
    }

    unsigned& operator()(size_t i, size_t j) {
        return get(i, j);
    }

    unsigned& get(size_t i, size_t j) {
        return data[i * BoardWidth + j];
    }

    auto getWidth() {
        return BoardWidth;
    }

    auto getHeight() {
        return BoardWidth;
    }

    void make_steps(size_t num) {
        while (num > 0) {
            auto cur = num;
            auto max = kStatPeriod - steps % kStatPeriod;
            if (cur > max) {
                cur = max;
            }
            make_steps_(cur);
            num -= cur;
        }
    }

    void make_steps_(size_t num) {
        std::unique_lock main_lock(main_mx);
        auto start_time = std::chrono::system_clock::now();
        for (size_t j = 0; j != num; ++j) {
            for (size_t i = 0; i < BoardWidth; i += 2 * MutexLockWidth) {
                tasks.push_back(i);
            }
            for (size_t i = MutexLockWidth; i < BoardWidth; i += 2 * MutexLockWidth) {
                tasks.push_back(i);
            }
        }
        cv_start.notify_all();
        while (!tasks.empty()) {
            cv_finish.wait(main_lock);
        }
        size_t num_of_not_alive = 0;
        for (auto && bot : bots) {
            if (!bot->isAlive()) {
                ++num_of_not_alive;
            }
        }
        if (num_of_not_alive * 2 > bots.size()) {
            comperess();
        }
        for (auto && nbots : new_bots) {
            for (auto && bot : nbots) {
                if (get(bot->x, bot->y) != None) {
                    continue;
                }
                get(bot->x, bot->y) = bots.size();
                bots.push_back(bot);
            }
            nbots.clear();
        }
        auto end_time = std::chrono::system_clock::now();
        steps += num;
        if (steps % kStatPeriod == 0) {
            auto diff = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();
            if (diff == 0) {
                diff = 1;
            }
            stat_data[kStatBPS].push(num * 1000000000 * (bots.size() - num_of_not_alive) / diff);
            stat_data[kStatAlive].push(bots.size() - num_of_not_alive);
            for (unsigned i = 0; i != Bot::ComandsNum; ++i) {
                stat_data[kStatNum + i].push(Bot::getStat(i) / kStatPeriod);
            }
        }
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
        unsigned new_x = (bot.x + dx) % BoardWidth;
        unsigned new_y = (bot.y + dy) % BoardWidth;
        return get(new_x, new_y);
    }

    void check_and_go(Bot &bot, int dx, int dy) {
        unsigned new_x = (bot.x + dx) % BoardWidth;
        unsigned new_y = (bot.y + dy) % BoardWidth;
        if (get(new_x, new_y) != None) {
            return;
        }
        get(new_x, new_y) = get(bot.x, bot.y);
        get(bot.x, bot.y) = None;
        bot.x = new_x;
        bot.y = new_y;
    }

    void new_bot(size_t thread_id, const Bot &bot, int dx, int dy, unsigned char acc) {
        unsigned new_x = (bot.x + dx) % BoardWidth;
        unsigned new_y = (bot.y + dy) % BoardWidth;
        if (get(new_x, new_y) != None) {
            return;
        }
        new_bots[thread_id].push_back(QSharedPointer<Bot>::create(bot, new_x, new_y, acc));
    }

    void new_bot(size_t thread_id, const Bot &bot1, const Bot &bot2, int dx, int dy, unsigned char acc) {
        unsigned new_x = (bot1.x + dx) % BoardWidth;
        unsigned new_y = (bot1.y + dy) % BoardWidth;
        if (get(new_x, new_y) != None) {
            return;
        }
        new_bots[thread_id].push_back(QSharedPointer<Bot>::create(bot1, bot2, new_x, new_y, acc));
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
        unsigned state = dist(2 * BoardWidth / 5, BoardWidth / 2, bot.x, bot.y);
        if (!resource_zone || state > BoardWidth / 8) {
            return xorshf96() % 3;
        } else {
            return std::min(3 * (BoardWidth / 8 - state), 30ul);
        }
    }

    unsigned minerals(Bot &bot) {
        unsigned state = dist(3 * BoardWidth / 5, BoardWidth / 2, bot.x, bot.y);
        if (!resource_zone || state > BoardWidth / 8) {
            return xorshf96() % 2;
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
    std::vector<Graphic::Data> stat_data;

private:
    std::vector<unsigned> data;
    std::vector<QSharedPointer<Bot>> bots;

    std::array<std::mutex, BoardWidth / MutexLockWidth> mxs;
    std::mutex main_mx;
    std::condition_variable cv_start;
    std::condition_variable cv_finish;
    std::vector<std::thread> threads;
    size_t running_threads = 0;
    bool killed = false;
    std::vector<size_t> tasks;

    std::vector<std::vector<QSharedPointer<Bot>>> new_bots;

    unsigned steps = 0;
};

#endif // BOARD_H
