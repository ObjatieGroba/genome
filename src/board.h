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

constexpr auto kStatPeriod = 128;
constexpr auto kStatAlive = 0;
constexpr auto kStatBPS = 1;
constexpr auto kStatMutations = 2;
constexpr auto kStatProcessDepth = 3;
constexpr auto kStatNum = 4;

unsigned get_thread_id();
void set_thread_id(unsigned id);
namespace Stat {
    void init(unsigned num);
    void add_threads(unsigned num);

    void inc(unsigned id);
    void inc(unsigned id, unsigned add);
    unsigned get(unsigned id);
}

template <class Bot, size_t BoardWidth = 256, size_t MutexLockWidth = 16>
class Board : public std::enable_shared_from_this<Board<Bot, BoardWidth, MutexLockWidth>>
{
public:
    Board() : data(BoardWidth * BoardWidth, None) {
        std::cerr << sizeof(Bot) << std::endl;
        static_assert(BoardWidth % MutexLockWidth == 0);
        refill(QSharedPointer<Bot>::create());
        stat_data.emplace_back("Alive bots");
        stat_data.emplace_back("Bots per second");
        stat_data.emplace_back("Mutations per 100 new bots");
        stat_data.emplace_back("Processing Depth");
        for (unsigned i = 0; i != Bot::CommandsNum; ++i) {
            stat_data.emplace_back(Bot::CommandNames[i] + (" (" + std::to_string(i) + ")"));
            stat_data.back().push(0);
        }
        for (unsigned i = 0; i != Bot::kVarTypesNum; ++i) {
            stat_data.emplace_back(std::string("Val ") + Bot::VarTypeNames[i] + " (" + std::to_string(i) + ")");
            stat_data.back().push(0);
        }
        Stat::init(Bot::CommandsNum + Bot::kVarTypesNum + kStatNum);
    }

    ~Board() {
        std::unique_lock lock(main_mx);
        killed = true;
        running_threads += threads.size();
        cv_start.notify_all();
        while (killed) {
            cv_finish.wait(lock);
        }
        for (auto && thread : threads) {
            thread.join();
        }
    }

    void random() {
        std::unique_lock lock(main_mx);
        if (running_threads > 0) {
            throw std::runtime_error("Do not permitted while other threads running");
        }
        for (auto && mx : mxs) {
            mx.lock();
        }
        for (auto && bot : bots) {
            bot->random();
        }
        for (auto && mx : mxs) {
            mx.unlock();
        }
    }

    void BotStatInc(unsigned id) {
        if (get_current_time() % kStatPeriod == 0) {
            Stat::inc(id);
        }
    }
    void BotStatInc(unsigned id, unsigned add) {
        if (get_current_time() % kStatPeriod == 0) {
            Stat::inc(id, add);
        }
    }
    void BotVarTypesStatInc(unsigned id) {
        BotStatInc(id + Bot::CommandsNum + kStatNum);
    }
    unsigned BotVarTypesStatGet(unsigned id) {
        return Stat::get(id + Bot::CommandsNum + kStatNum);
    }
    void BotCommandStatInc(unsigned id) {
        BotStatInc(id + kStatNum);
    }
    unsigned BotCommandStatGet(unsigned id) {
        return Stat::get(id + kStatNum);
    }

    void refill(const QSharedPointer<Bot> &bot) {
        if (!bot) {
            throw std::runtime_error("No bot");
        }
        std::unique_lock lock(main_mx);
        if (running_threads > 0) {
            throw std::runtime_error("Do not permitted while other threads running");
        }
        for (auto && mx : mxs) {
            mx.lock();
        }
        bots.clear();
        for (auto && nb : new_bots) {
            nb.clear();
        }
        std::fill(data.begin(), data.end(), None);
        for (unsigned i = 0; i != BoardWidth * BoardWidth / 16; ++i) {
            unsigned x = xorshf96();
            unsigned y = xorshf96();
            x %= BoardWidth;
            y %= BoardWidth;
            if (get(x, y) != None) {
                continue;
            }
            get(x, y) = bots.size();
            bots.push_back(QSharedPointer<Bot>::create(*this, *bot, x, y, 0));
            bots.back()->way = xorshf96() % 4;
        }
        for (auto && mx : mxs) {
            mx.unlock();
        }
    }

    void part_kill() {
        std::unique_lock lock(main_mx);
        if (running_threads > 0) {
            throw std::runtime_error("Do not permitted while other threads running");
        }
        for (auto && mx : mxs) {
            mx.lock();
        }
        for (auto && bot : bots) {
            if (xorshf96() % 2 == 0) {
                bot->alive = false;
            }
        }
        for (auto && mx : mxs) {
            mx.unlock();
        }
    }

    void run_threads(size_t num) {
        std::unique_lock main_lock(main_mx);
        if (threads.size() + num > BoardWidth / MutexLockWidth / 2) {
            std::cerr << "Too many threads\n";
        }
        Stat::add_threads(num);
        auto self = this->shared_from_this();
        while (num-- > 0) {
            new_bots.emplace_back();
            threads.emplace_back([self, tid=threads.size()]() {
                set_thread_id(tid);
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
                        size_t wmutex = start_width / MutexLockWidth;
                        std::unique_lock lock1(self->mxs[wmutex % self->mxs.size()]);
                        std::unique_lock lock2(self->mxs[(wmutex + 1) % self->mxs.size()]);
                        for (size_t i = 0; i != BoardWidth; ++i) {
                            for (size_t j = 0; j != MutexLockWidth; ++j) {
                                auto id = self->get(start_width + j, i);
                                if (id != None) {
                                    self->bots[id]->Step(*self);
                                }
                            }
                        }
                        lock1.unlock();
                        start_width += MutexLockWidth;
                        std::unique_lock lock3(self->mxs[(wmutex + 2) % self->mxs.size()]);
                        for (size_t i = 0; i != BoardWidth; ++i) {
                            for (size_t j = 0; j != MutexLockWidth; ++j) {
                                auto id = self->get(start_width + j, i);
                                if (id != None) {
                                    self->bots[id]->Step(*self);
                                }
                            }
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
            make_steps_();
            num -= 1;
        }
    }

    void make_steps_() {
        std::unique_lock main_lock(main_mx);
        std::chrono::time_point<std::chrono::system_clock> start_time;
        if (steps % kStatPeriod == 0) {
            start_time = std::chrono::system_clock::now();
        }
        for (size_t i = 0; i < BoardWidth; i += 2 * MutexLockWidth) {
            /// Thread should process all from i to i + 2 * MutexLockWidth
            tasks.push_back(i);
        }
        cv_start.notify_all();
        while (!tasks.empty()) {
            cv_finish.wait(main_lock);
        }
        size_t num_of_not_alive = 0;
        size_t num_of_not_on_board = 0;
        for (auto && bot : bots) {
            if (!bot->isAlive()) {
                ++num_of_not_alive;
            }
            if (bot->x == std::numeric_limits<decltype(bot->x)>::max()) {
                ++num_of_not_on_board;
            }
        }
        auto num_of_alive = bots.size() - num_of_not_alive;
        if (num_of_not_on_board * 2 > bots.size()) {
            comperess();
        }
        size_t num_of_new_bots = 0;
        for (auto && nbots : new_bots) {
            num_of_new_bots += nbots.size();
            for (auto && bot : nbots) {
                if (get(bot->x, bot->y) != None) {
                    continue;
                }
                get(bot->x, bot->y) = bots.size();
                bots.push_back(bot);
            }
            nbots.clear();
        }
        if (steps % kStatPeriod == 0) {
            auto end_time = std::chrono::system_clock::now();
            auto diff = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();
            if (diff == 0) {
                diff = 1;
            }
            stat_data[kStatBPS].push(1000000000 * num_of_alive / diff);
            stat_data[kStatAlive].push(num_of_alive);
            for (unsigned i = 0; i != Bot::CommandsNum; ++i) {
                stat_data[kStatNum + i].push(BotCommandStatGet(i));
            }
            for (unsigned i = 0; i != Bot::kVarTypesNum; ++i) {
                stat_data[kStatNum + Bot::CommandsNum + i].push(BotVarTypesStatGet(i));
            }
            if (num_of_new_bots == 0) {
                stat_data[kStatMutations].push(0);
            } else {
                stat_data[kStatMutations].push((Stat::get(kStatMutations) * 100) / num_of_new_bots);
            }
            if (num_of_alive != 0) {
                stat_data[kStatProcessDepth].push(Stat::get(kStatProcessDepth) / num_of_alive);
            } else {
                stat_data[kStatProcessDepth].push(0);
            }
        }
        ++steps;
    }

    void stat() {
        comperess();
        std::cerr << bots.size() << std::endl;
        double sum = 0;
        std::vector<unsigned> a(Bot::CommandsNum);
        for (auto && bot : bots) {
            sum += bot->energy;
            for (auto g : bot->memory) {
                ++a[g % a.size()];
            }
        }
        std::cerr << sum / bots.size() << std::endl;
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
            if (bots[i]->x == std::numeric_limits<decltype(bots[i]->x)>::max() &&
                bots[i]->y == std::numeric_limits<decltype(bots[i]->x)>::max()) {
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

    void new_bot(const Bot &bot, int dx, int dy, unsigned char acc) {
        unsigned new_x = (bot.x + dx) % BoardWidth;
        unsigned new_y = (bot.y + dy) % BoardWidth;
        if (get(new_x, new_y) != None) {
            return;
        }
        new_bots[get_thread_id()].push_back(QSharedPointer<Bot>::create(*this, bot, new_x, new_y, acc));
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

    auto get_current_time() {
        return steps;
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
