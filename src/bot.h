#ifndef BOT_H
#define BOT_H

#include <array>
#include <iostream>
#include <sstream>

#include "board.h"

enum {
    Top = 0,
    Right = 1,
    Bottom = 2,
    Left = 3
};

enum {
    TypeKill = 0,
    TypeSynthesis = 1,
    TypeMinerals = 2,
    TypeGiveEnergy = 3,
    TypeGiveMinerals = 4,
};

class Bot {
public:
    Bot() {}

private:
    template <class Board>
    void mutate(Board &board, unsigned accuracy) {
        if (accuracy > 255) {
            throw std::runtime_error("Bad accuracy");
        }
        accuracy = 2 + accuracy / 16;
        while ((xorshf96() % accuracy) == 0) {
            auto point = xorshf96();
            memory[point % memory.size()] = xorshf96();
            board.BotStatInc(kStatMutations);
            if (xorshf96() % accuracy == 0) {
                ++point;
                memory[point % memory.size()] = xorshf96();
                board.BotStatInc(kStatMutations);
            }
            accuracy += 1;
        }
    }

public:
    template <class Board>
    Bot(Board & board, const Bot& other, unsigned x_, unsigned y_, unsigned acc)
        : memory(other.memory), x(x_), y(y_), way(other.way), max_live_time(board.get_current_time() + MaxAge) {
        mutate(board, acc);
    }

    std::string info(bool olny_genome=false) const {
        std::stringstream ss;
        if (!olny_genome) {
            ss << "Way: " << (unsigned)way << "\n";
            ss << "Energy: " << energy << "\n";
            ss << "Minerals: " << (unsigned)minerals << "\n";
            ss << "DeadTime: " << max_live_time << "\n";
            ss << "Pointer: " << (unsigned)pointer << "\n";
            ss << "Messages: ";
            for (auto msg : msgs) {
                ss << (unsigned)msg << ' ';
            }
            ss << "\n";
            ss << "\nMemory: ";
            for (auto g : memory) {
                ss << (unsigned)g % CommandsNum << ' ';
            }
        } else {
            for (auto g : memory) {
                ss << (unsigned)g << ' ';
            }
        }
        ss << "\n";
        return ss.str();
    }

    bool isSimilar(const Bot & other) const {
        char found = 0;
        for (unsigned i = 0; i != memory.size(); ++i) {
            if (memory[i] != other.memory[i]) {
                if (found == 3) {
                    return false;
                }
                ++found;
            }
        }
        return true;
    }

    template <class Board>
    Bot * getFrontBot(Board &board) {
        unsigned data;
        switch (way) {
            case Top:
                data = board.get(*this, 0, -1);
                break;
            case Right:
                data = board.get(*this, 1, 0);
                break;
            case Bottom:
                data = board.get(*this, 0, 1);
                break;
            case Left:
                data = board.get(*this, -1, 0);
                break;
        }
        if (data == None) {
            return nullptr;
        }
        return board.getBot(data).get();
    }

    /// Variables
    static constexpr auto kVarTypesNum = 19;
    static constexpr std::array<const char *, kVarTypesNum> VarTypeNames {
            "ByteConstant",
            "Energy",
            "Minerals",
            "AliveFront",
            "RelativeFront",
            "MemAbs",
            "MemRel",
            "Way",
            "SinceAttack",
            "SinceMsg",
            "LastSynthesized",
            "LastMinerals",
            "Age",
            "MsgTop",
            "MsgRight",
            "MsgBottom",
            "MsgLeft",
            "UIntConstant",
            "EmptyFront",
    };
    template <class Board>
    unsigned int readVariable(Board &board) {
        unsigned res;
        auto var_type = memory[pointer] % kVarTypesNum;
        board.BotVarTypesStatInc(var_type);
        switch (var_type) {
            case 0:  /// Byte Constant
                ++pointer; pointer %= memory.size();
                res =  memory[pointer]; break;
            case 1:  /// Energy
                res = std::max(0, energy); break;
            case 2:  /// Minerals
                res = minerals; break;
            case 3:  /// Front bot alive
            {
                auto bot = getFrontBot(board);
                if (bot == nullptr || !bot->isAlive()) {
                    res = 0;
                } else {
                    res = 1;
                }
            }
                break;
            case 4:  /// Front bot relative
            {
                auto bot = getFrontBot(board);
                if (bot == nullptr || !bot->isSimilar(*this)) {
                    res = 0;
                } else {
                    res = 1;
                }
            }
                break;
            case 5:  /// Memory Absolute
                ++pointer; pointer %= memory.size();
                res = memory[memory[pointer] % memory.size()]; break;
            case 6:  /// Memory Relative
                ++pointer; pointer %= memory.size();
                res = memory[(pointer + memory[pointer]) % memory.size()]; break;
            case 7:  /// Current Way
                res = way; break;
            case 8:  /// Cycles since last attack
                res = board.get_current_time() - last_under_attack; break;
            case 9:  /// Cycles since last message
                res = board.get_current_time() - last_msg_time; break;
            case 10:  /// Last synthesized
                res = lastSynthesized; break;
            case 11:  /// Last minerals
                res = lastMinerals; break;
            case 12:  /// Age
                res = max_live_time - board.get_current_time(); break;
            case 13:  /// Msg
            case 14:
            case 15:
            case 16:
                res = msgs[var_type - 13]; break;
            case 17:  /// UInt Constant
                ++pointer; pointer %= memory.size();
                res = memory[pointer];
                ++pointer; pointer %= memory.size();
                res <<= CHAR_BIT; res += memory[pointer];
                ++pointer; pointer %= memory.size();
                res <<= CHAR_BIT; res += memory[pointer];
                ++pointer; pointer %= memory.size();
                res <<= CHAR_BIT; res += memory[pointer];
                break;
            case 18:  /// Empty front cell
                res = getFrontBot(board) == nullptr ? 1 : 0; break;
            default:
                throw std::runtime_error("Unknown variable type");
        }
        ++pointer; pointer %= memory.size();
        return res;
    }

    /// Actions
    template <class Board>
    void Forward(Board &board) {
        --energy;
        switch (way) {
        case Top:
            board.check_and_go(*this, 0, -1);
            break;
        case Right:
            board.check_and_go(*this, 1, 0);
            break;
        case Bottom:
            board.check_and_go(*this, 0, 1);
            break;
        case Left:
            board.check_and_go(*this, -1, 0);
            break;
        }
    }

    template <class Board>
    void Synthesise(Board &board) {
        auto e = lastSynthesized = board.syntes(*this);
        if (minerals > 0) {
            --minerals;
            e *= 10;
        }
        energy += e;
        type = TypeSynthesis;
    }

    template <class Board>
    void MakeMinerals(Board &board) {
        auto got = board.minerals(*this);
        if (got + minerals > 255) {
            got = 255 - minerals;
        }
        minerals += (lastMinerals = got);
        type = TypeMinerals;
    }

    template <class Board>
    void EatFront(Board &board) {
        unsigned * data;
        switch (way) {
            case Top:
                data = &board.get(*this, 0, -1);
                break;
            case Right:
                data = &board.get(*this, 1, 0);
                break;
            case Bottom:
                data = &board.get(*this, 0, 1);
                break;
            case Left:
                data = &board.get(*this, -1, 0);
                break;
        }
        if (*data == None) {
            return;
        }
        type = TypeKill;
        auto & bot = board.getBot(*data);
        if (bot->isAlive() && bot->energy > 1024) {
            bot->energy -= 1024;
            bot->last_under_attack = board.get_current_time();
            return;
        }
        auto max_minerals = 255 - minerals;
        if (bot->minerals < max_minerals) {
            max_minerals = bot->minerals;
        }
        minerals += max_minerals;
        energy += std::max(0, bot->energy) / 2;
        bot->energy = 0;
        bot->minerals = 0;
        bot->alive = false;
        bot->x = std::numeric_limits<decltype(x)>::max();
        bot->y = std::numeric_limits<decltype(y)>::max();
        *data = None;
    }

    template <class Board>
    void GiveEnergy(Board &board) {
        auto bot = getFrontBot(board);
        type = TypeGiveEnergy;
        auto to_give = readVariable(board);
        if (to_give > energy) {
            to_give = energy;
        }
        energy -= to_give;
        if (bot != nullptr) {
            bot->energy += to_give * 0.9;
        }
    }

    template <class Board>
    void GiveMinerals(Board &board) {
        auto bot = getFrontBot(board);
        type = TypeGiveMinerals;
        auto to_give = readVariable(board);
        if (to_give > minerals) {
            to_give = minerals;
        }
        energy -= 1 + (to_give / 16);
        if (bot != nullptr) {
            if (255 - bot->minerals < to_give) {
                to_give = 255 - bot->minerals;
            }
            bot->minerals += to_give;
        }
        minerals -= to_give;
    }

    template <class Board>
    void SendMsg(Board &board) {
        auto bot = getFrontBot(board);
        --energy;
        if (bot != nullptr) {
            bot->msgs[way] = readVariable(board);
            bot->last_msg_time = board.get_current_time();
        }
    }

    template <class Board>
    void MakeNewBot(Board &board) {
        auto accuracy = std::min(readVariable(board), 255u);
        int need_energy = StartBotEnergy + accuracy;
        if (energy < need_energy) {
            energy = 0;
            return;
        }
        energy -= need_energy;
        switch (way) {
            case Top:
                board.new_bot(*this, 0, -1, accuracy);
                break;
            case Right:
                board.new_bot(*this, 1, 0, accuracy);
                break;
            case Bottom:
                board.new_bot(*this, 0, 1, accuracy);
                break;
            case Left:
                board.new_bot(*this, -1, 0, accuracy);
                break;
        }
    }

    /// Branches
    template <class Board>
    void If(Board &board, unsigned if_type) {
        auto left = readVariable(board);
        auto right = readVariable(board);
        bool truth;
        switch (if_type) {
            case 0:  /// Greater or equal
                truth = left >= right; break;
            case 1:  /// Equal
                truth = left == right; break;
            case 2:  /// Less
                truth = left < right; break;
            default:
                throw std::runtime_error("No such if type");
        }
        AbsoluteJump(board, !truth);
    }

    /// Extra
    template <class Board>
    void RelativeJump(Board &board, bool skip = false) {
        auto size = readVariable(board);
        if (!skip) {
            pointer += size;
            pointer %= memory.size();
        }
    }

    template <class Board>
    void AbsoluteJump(Board &board, bool skip = false) {
        auto size = readVariable(board);
        if (!skip) {
            pointer = size % memory.size();
        }
    }

    template <class Board>
    void Store(Board &board, bool abs) {
        auto addr = readVariable(board);
        auto val = readVariable(board);
        if (!abs) {
            addr += pointer;
        }
        memory[addr % memory.size()] = val;
    }

    template <class Board>
    void IncStore(Board &board, bool abs) {
        auto addr = readVariable(board);
        auto val = readVariable(board);
        if (!abs) {
            addr += pointer;
        }
        memory[addr % memory.size()] += val;
    }

    template <class Board>
    void CopyStore(Board &board, bool abs) {
        auto addr_from = readVariable(board);
        auto addr_to = readVariable(board);
        if (!abs) {
            addr_from += pointer;
            addr_to += pointer;
        }
        memory[addr_to % memory.size()] = memory[addr_from % memory.size()];
    }

    static constexpr unsigned StartBotEnergy = 100;
    static constexpr unsigned CommandsNum = 22;
    static constexpr unsigned MaxAge = 1u << 13u;
    static constexpr unsigned MaxDepth = 64;
    static constexpr unsigned genomeSize = 128;
    static constexpr std::array<const char *, CommandsNum> CommandNames {
        "Lazy",
        "Forward",
        "Left",
        "Right",
        "Synthesise",
        "MakeMinerals",
        "EatFront",
        "GiveEnergy",
        "GiveMinerals",
        "SendMsg",
        "MakeNewBot",
        "IfGEq",
        "IfEq",
        "IfLe",
        "RelativeJump",
        "AbsoluteJump",
        "Store",
        "StoreAbs",
        "IncStore",
        "IncStoreAbs",
        "CopyStore",
        "CopyStoreAbs",
    };

    template <class Board>
    void Step(Board &board) {
        board.BotStatInc(kStatProcessDepth, MakeStep(board));
    }

    template <class Board>
    unsigned MakeStep(Board &board) {
        if (!isAlive()) {
            return 0;
        }
        if (board.get_current_time() == max_live_time) {
            alive = false;
            return 0;
        }
        unsigned num = MaxDepth;
        while (--num) {
            if (energy <= 0) {
                alive = false;
                return MaxDepth - num;
            }
            --energy;
            auto cmd = memory[pointer] % CommandsNum;
            // std::cerr << CommandNames[cmd] << ' ' << pointer << '\n';
            board.BotCommandStatInc(cmd);
            ++pointer; pointer %= memory.size();
            switch (cmd) {
                case 0:  /// Lazy
                    return MaxDepth - num;
                case 1:  /// Forward step
                    Forward(board);
                    return MaxDepth - num;
                case 2:  /// Left rotate
                    way += Left;
                    way %= 4;
                    break;
                case 3:  /// Right rotate
                    way += Right;
                    way %= 4;
                    break;
                case 4:  /// Synthesise
                    Synthesise(board);
                    return MaxDepth - num;
                case 5:  /// MakeMinerals
                    MakeMinerals(board);
                    return MaxDepth - num;
                case 6:  /// Eat Front
                    EatFront(board);
                    return MaxDepth - num;
                case 7:  /// GiveEnergy
                    GiveEnergy(board);
                    break;
                case 8:  /// GiveMinerals
                    GiveMinerals(board);
                    break;
                case 9:  /// SendMsg
                    SendMsg(board);
                    break;
                case 10:  /// MakeNewBot
                    MakeNewBot(board);
                    return MaxDepth - num;
                case 11:  /// If GEq
                case 12:  ///    Eq
                case 13:  ///    Le
                    If(board, cmd - 11);
                    break;
                case 14:  /// RelativeJump
                    RelativeJump(board);
                    break;
                case 15:  /// AbsoluteJump
                    AbsoluteJump(board);
                    break;
                case 16:  /// Store
                case 17:
                    Store(board, cmd - 16);
                    break;
                case 18:  /// IncStore
                case 19:
                    IncStore(board, cmd - 18);
                    break;
                case 20:  /// CopyStore
                case 21:
                    CopyStore(board, cmd - 20);
                    break;
                default:
                    throw std::runtime_error("Unknown command");
            }
        }
        return MaxDepth - num;
    }

    bool isAlive() const {
        return alive;
    }

    // std::array<unsigned char, genomeSize> memory{5, 4, 4, 12, 18, 0, 1, 0, 22, 12, 4, 0, 1, 0, 17, 15, 0, 0, 2, 15, 0, 0, 13, 1, 0, 200, 0, 0, 10, 0, 16, 15, 0, 0};
    std::array<unsigned char, genomeSize> memory{219, 137, 92, 188, 151, 0, 1, 133, 22, 12, 117, 0, 1, 0, 255, 160, 103, 233, 81, 155, 193, 146, 13, 1, 0, 135, 0, 255, 10, 0, 16, 114, 213, 67, 155, 87, 69, 29, 92, 209, 152, 243, 80, 112, 203, 168, 139, 83, 177, 238, 99, 139, 72, 214, 228, 166, 194, 241, 81, 236, 169, 124, 176, 47};

    int energy = StartBotEnergy;
    unsigned x;
    unsigned y;
    unsigned last_msg_time = 0;
    unsigned last_under_attack = 0;
    unsigned lastSynthesized;
    unsigned max_live_time = 0;
    std::array<unsigned char, 4> msgs{};
    unsigned char pointer = 0;
    unsigned char way = Top;
    unsigned char minerals = 0;
    unsigned char lastMinerals;
    unsigned char type = 0;

    bool alive = true;
    bool isSelected = false;
};

#endif // BOT_H
