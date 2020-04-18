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

class Bot {
public:
    Bot() {}
    Bot(unsigned x_, unsigned y_) : x(x_), y(y_) {
        memory = genome;
    }

private:
    void mutate(unsigned acc) {
        auto curr = 2 + acc;
        while ((xorshf96() % curr) == 0) {
            auto point = xorshf96();
            genome[point % genome.size()] = xorshf96();
            if (xorshf96() % (2 + acc) == 0) {
                ++point;
                genome[point % genome.size()] = xorshf96();
            }
            curr += 2;
        }
    }

public:

    Bot(const Bot& other, unsigned x_, unsigned y_, unsigned acc) : genome(other.genome), x(x_), y(y_), way(other.way) {
        mutate(acc);
        memory = genome;
    }

    Bot(const Bot& other1, const Bot& other2, unsigned x_, unsigned y_, unsigned acc) : x(x_), y(y_), way(other1.way) {
        unsigned r = 0;
        bool first = false;
        for (unsigned i = 0; i != genome.size(); ++i) {
            if (r == 0) {
                r = xorshf96() % genomeSize;
                first = !first;
            }
            --r;
            genome[i] = first ? other1.genome[i] : other2.genome[i];
        }
        mutate(acc);
        memory = genome;
    }

    std::string info(bool olny_genome=false) const {
        std::stringstream ss;
        if (!olny_genome) {
            ss << "Way: " << way << "\n";
            ss << "Energy: " << energy << "\n";
            ss << "Minerals: " << minerals << "\n";
            ss << "Age: " << age << "\n";
            ss << "Pointer: " << pointer << "\n";
            ss << "Messages: ";
            for (auto msg : msgs) {
                ss << msg << ' ';
            }
            ss << "\n";
            ss << "Saved: " << saved.get() << "\n";
            ss << "Genome: ";
            for (auto g : genome) {
                ss << (unsigned)g % ComandsNum << ' ';
            }
            ss << "\nMemory: ";
            for (auto g : memory) {
                ss << (unsigned)g % ComandsNum << ' ';
            }
        } else {
            for (auto g : genome) {
                ss << (unsigned)g << ' ';
            }
        }
        ss << "\n";
        return ss.str();
    }

    bool similar(const Bot & other) const {
        char found = 0;
        for (unsigned i = 0; i != genome.size(); ++i) {
            if (genome[i] != other.genome[i]) {
                if (found == 3) {
                    return false;
                }
                ++found;
            }
        }
        return true;
    }

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
    void CheckFront(Board &board) {
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
            pointer += memory[(pointer + 1) % memory.size()];
        } else if (similar(*board.getBot(data))) {
            pointer += memory[(pointer + 2) % memory.size()];
        } else {
            pointer += memory[(pointer + 3) % memory.size()];
        }
    }

    template <class Board>
    void SaveGenome(Board &board) {
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
            saved.reset();
        } else {
            saved = board.getBot(data);
        }
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
        type = 0;
        auto & bot = board.getBot(*data);
        if (bot->isAlive() && bot->energy > 255) {
            bot->energy -= 255;
            bot->last_under_attack = board.get_current_time();
            return;
        }
        minerals += bot->minerals;
        if (minerals > 255) {
            minerals = 255;
        }
        energy += std::max(0, bot->energy) / 2;
        bot->energy = 0;
        bot->minerals = 0;
        bot->alive = false;
        bot->x = -1;
        bot->y = -1;
        *data = None;
    }

    template <class Board>
    void GiveEnergy(Board &board) {
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
        type = 3;
        auto meta = std::min(energy, static_cast<int>(memory[(pointer + 1) % memory.size()]));
        energy -= meta;
        if (energy > 0) {
            board.getBot(*data)->energy += meta * 0.9;
        }
        ++pointer;
    }

    template <class Board>
    void SendMsg(Board &board) {
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
        energy -= 1;
        if (energy > 0) {
            board.getBot(*data)->msgs[way] = memory[(pointer + 1) % memory.size()];
            board.getBot(*data)->last_msg_time = board.get_current_time();
        }
        ++pointer;
    }

    template <class Board>
    void GiveMinerals(Board &board) {
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
        type = 4;
        auto todo = std::min(minerals, static_cast<unsigned>(memory[(pointer + 1) % memory.size()]) / 2);
        energy -= todo / 16;
        minerals -= todo;
        board.getBot(*data)->minerals += todo;
        ++pointer;
    }

    template <class Board>
    void NewBot(Board &board, size_t thread_id) {
        auto acc = memory[(pointer + 1) % memory.size()] % 4;
        ++pointer;
        int need_energy = 100 + 20 + acc * 20;
        if (energy < need_energy) {
            energy = 0;
            return;
        }
        energy -= need_energy;
        if (saved) {
            switch (way) {
            case Top:
                board.new_bot(thread_id, *this, *saved, 0, -1, acc);
                break;
            case Right:
                board.new_bot(thread_id, *this, *saved, 1, 0, acc);
                break;
            case Bottom:
                board.new_bot(thread_id, *this, *saved, 0, 1, acc);
                break;
            case Left:
                board.new_bot(thread_id, *this, *saved, -1, 0, acc);
                break;
            }
        } else {
            switch (way) {
            case Top:
                board.new_bot(thread_id, *this, 0, -1, acc);
                break;
            case Right:
                board.new_bot(thread_id, *this, 1, 0, acc);
                break;
            case Bottom:
                board.new_bot(thread_id, *this, 0, 1, acc);
                break;
            case Left:
                board.new_bot(thread_id, *this, -1, 0, acc);
                break;
            }
        }
    }

    void CheckEnergy() {
        ++pointer;
        pointer %= memory.size();
        if (energy > memory[pointer]) {
            pointer += 1;
        } else {
            pointer += memory[(pointer + 1) % memory.size()];
        }
    }

    void CheckWay() {
        ++pointer;
        pointer %= memory.size();
        if (way == memory[pointer] % 4) {
            pointer += 1;
        } else {
            pointer += memory[(pointer + 1) % memory.size()];
        }
    }

    void CheckSaved() {
        ++pointer;
        pointer %= memory.size();
        if (bool(saved) != (memory[pointer] % 2 == 0)) {
            pointer += 1;
        } else {
            pointer += memory[(pointer + 1) % memory.size()];
        }
    }

    void CheckMinerals() {
        ++pointer;
        pointer %= memory.size();
        if (minerals > memory[pointer]) {
            pointer += 1;
        } else {
            pointer += memory[(pointer + 1) % memory.size()];
        }
    }

    void CheckMsg() {
        ++pointer;
        pointer %= memory.size();
        auto mway = memory[pointer] % 4;
        if ((msgs[mway] >> CHAR_BIT) != 0) {
            pointer += 1;
        } else {
            pointer += memory[(pointer + 1) % memory.size()];
        }
    }

    void ReadMsg() {
        ++pointer;
        pointer %= memory.size();
        auto mway = memory[pointer] % 4;
        ++pointer;
        pointer %= memory.size();
        if (msgs[mway] > memory[pointer]) {
            pointer += 1;
        } else {
            pointer += memory[(pointer + 1) % memory.size()];
        }
        msgs[mway] = 0;
    }

    void CheckAge() {
        ++pointer;
        pointer %= memory.size();
        if ((age * 256) / MaxAge > memory[pointer]) {
            pointer += 1;
        } else {
            pointer += memory[(pointer + 1) % memory.size()];
        }
    }

    void CheckLastSyntes() {
        ++pointer;
        pointer %= memory.size();
        if (lastSyntes > memory[pointer]) {
            pointer += 1;
        } else {
            pointer += memory[(pointer + 1) % memory.size()];
        }
    }

    template <class Board>
    void CheckAttack(Board &board) {
        ++pointer;
        pointer %= memory.size();
        auto diff = board.get_current_time() - last_under_attack;
        if (diff > (memory[pointer]) % 4) {
            pointer += 1;
        } else {
            pointer += memory[(pointer + 1) % memory.size()];
        }
    }

    void CheckLastMinerals() {
        ++pointer;
        pointer %= memory.size();
        if (lastMinerals > memory[pointer]) {
            pointer += 1;
        } else {
            pointer += memory[(pointer + 1) % memory.size()];
        }
    }

    void CheckStore(bool abs) {
        ++pointer;
        pointer %= memory.size();
        unsigned addr = memory[pointer] % memory.size();
        if (!abs) {
            addr += pointer;
            addr %= memory.size();
        }
        ++pointer;
        pointer %= memory.size();
        if (memory[addr] > memory[pointer]) {
            pointer += 1;
        } else {
            pointer += memory[(pointer + 1) % memory.size()];
        }
    }

    void Store(bool abs) {
        ++pointer;
        pointer %= memory.size();
        unsigned addr = memory[pointer] % memory.size();
        if (!abs) {
            addr += pointer;
            addr %= memory.size();
        }
        ++pointer;
        pointer %= memory.size();
        memory[addr] = memory[pointer];
    }

    void IncStore(bool abs) {
        ++pointer;
        pointer %= memory.size();
        unsigned addr = memory[pointer] % memory.size();
        if (!abs) {
            addr += pointer;
            addr %= memory.size();
        }
        ++pointer;
        pointer %= memory.size();
        memory[addr] += memory[pointer];
    }

    void CopyStore(bool abs) {
        ++pointer;
        pointer %= memory.size();
        unsigned addrfrom = memory[pointer] % memory.size();
        if (!abs) {
            addrfrom += pointer;
            addrfrom %= memory.size();
        }
        ++pointer;
        pointer %= memory.size();
        unsigned addrto = memory[pointer] % memory.size();
        if (!abs) {
            addrto += pointer;
            addrto %= memory.size();
        }
        memory[addrto] += memory[addrfrom];
    }

    void MsgToStore(bool abs) {
        ++pointer;
        pointer %= memory.size();
        auto mway = memory[pointer] % 4;
        ++pointer;
        pointer %= memory.size();
        unsigned addr = memory[pointer] % memory.size();
        if (!abs) {
            addr += pointer;
            addr %= memory.size();
        }
        memory[addr] = msgs[mway];
        msgs[mway] = 0;
    }

    template <class Board>
    void CheckAlive(Board &board) {
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
        if (*data == None || !board.getBot(*data)->isAlive()) {
            pointer += 1;
        } else {
            pointer += memory[(pointer + 1) % memory.size()];
        }
    }


    template <class Board>
    void Syntes(Board &board) {
        auto e = lastSyntes = board.syntes(*this);
        if (minerals > 0) {
            --minerals;
            e *= 10;
        }
        energy += e;
        type = 1;
    }

    template <class Board>
    void Minerals(Board &board) {
        minerals += (lastMinerals = board.minerals(*this));
        if (minerals > 255) {
            minerals = 255;
        }
        type = 2;
    }

    static constexpr unsigned ComandsNum = 38;
    static constexpr unsigned MaxAge = 1u << 13;
    static constexpr unsigned MaxDepth = 32;
    static constexpr unsigned genomeSize = 64;
    static constexpr unsigned memorySize = genomeSize;
    static std::array<unsigned, ComandsNum> stats;
    static constexpr std::array<const char *, ComandsNum> ComandNames {
        "Lazy",
        "Forward",
        "Left",
        "Right",
        "Check",
        "RelJmp",
        "Eat",
        "NewBot",
        "CheckEnergy",
        "Syntes",
        "GiveEnergy",
        "Minerals",
        "GiveMinerals",
        "CheckAlive",
        "SendMsg",
        "CheckMsg",
        "AbsJmp",
        "CheckAge",
        "CheckMinerals",
        "StoreAbs",
        "StoreRel",
        "CheckStoreAbs",
        "CheckStoreRel",
        "IncStoreAbs",
        "IncStoreRel",
        "CopyStoreAbs",
        "CopyStoreRel",
        "MsgStoreAbs",
        "MsgStore",
        "CheckWay",
        "SaveGenome",
        "CheckSaved",
        "ResetSaved",
        "ClearMessage",
        "CheckLastSyn",
        "CheckLastMin",
        "ReadMsg",
        "CheckWasAttacked",
    };

    static auto getStat(unsigned i) {
        auto tmp = stats[i];
        stats[i] = 0;
        return tmp;
    }

    template <class Board>
    void Step(Board &board, size_t thread_id) {
        if (!isAlive()) {
            return;
        }
        ++age;
        if (age == MaxAge) {
            alive = false;
            return;
        }
        unsigned num = MaxDepth;
        while (--num) {
            ++pointer;
            if (pointer >= memory.size()) {
                pointer %= memory.size();
            }
            if (energy <= 0) {
                alive = false;
                return;
            }
            --energy;
            ++stats[memory[pointer] % ComandsNum];
            switch (memory[pointer] % ComandsNum) {
            case 0: /// Lazy
                return;
            case 1: /// Forward step
                Forward(board);
                return;
            case 2: /// Left rotate
                way += Left;
                way %= 4;
                break;
            case 3: /// Right rotate
                way += Right;
                way %= 4;
                break;
            case 4: /// Check Front objects. NoneJmp OtherJmp RelativeJmp
                CheckFront(board);
                break;
            case 5: /// Rel Jmp
                pointer += memory[(pointer + 1) % memory.size()];
                break;
            case 6: /// Eat Front
                EatFront(board);
                return;
            case 7: /// New bot
                NewBot(board, thread_id);
                return;
            case 8: /// Check Energy. NeedEergy NotJmp Yes
                CheckEnergy();
                break;
            case 9:
                Syntes(board);
                return;
            case 10:
                GiveEnergy(board);
                break;
            case 11:
                Minerals(board);
                return;
            case 12:
                GiveMinerals(board);
                break;
            case 13: /// Check Alive. YesJmp No
                CheckAlive(board);
                break;
            case 14:
                SendMsg(board);
                break;
            case 15:
                CheckMsg();
                break;
            case 16: /// Abs Jmp
                pointer = memory[(pointer + 1) % memory.size()] % memory.size();
                break;
            case 17:
                CheckAge();
                break;
            case 18: /// Check Minerals. NeedEnergy NotJmp Yes
                CheckMinerals();
                break;
            case 19:
            case 20:
                Store(memory[pointer] % 2);
                break;
            case 21:
            case 22:
                CheckStore(memory[pointer] % 2);
                break;
            case 23:
            case 24:
                IncStore(memory[pointer] % 2);
                break;
            case 25:
            case 26:
                CopyStore(memory[pointer] % 2);
                break;
            case 27:
            case 28:
                MsgToStore(memory[pointer] % 2);
                break;
            case 29:
                CheckWay();
                break;
            case 30:
                SaveGenome(board);
                break;
            case 31:
                CheckSaved();
                break;
            case 32:  /// Reset saved genome
                saved.reset();
                break;
            case 33:  /// Clear messages
                msgs.fill(0);
                break;
            case 34:
                CheckLastSyntes();
                break;
            case 35:
                CheckLastMinerals();
                break;
            case 36:
                ReadMsg();
                break;
            case 37:
                CheckAttack(board);
                break;
            }
        }
    }

    bool isAlive() const {
        return alive;
    }

    // std::array<unsigned char, 32> genome{6, 13, 17, 8, 250, 5, 7, 11, 18, 8, 11, 1, 5, 12, 2, 4, 9, 9, 9, 10, 1, 12, 9, 7, 9, 4, 9, 10, 3, 8, 8, 9};
    // std::array<unsigned char, 32> genome{4, 10, 3, 8, 250, 6, 7, 2, 18, 8, 9, 1, 5, 11, 9, 9, 9, 9, 9, 8, 10, 9, 1, 12, 11, 4, 9, 13, 10, 6, 7, 2};
    // std::array<unsigned char, 32> genome{4, 10, 3, 8, 250, 6, 7, 2, 18, 8, 9, 1, 5, 11, 9, 9, 9, 11, 9, 8, 10, 9, 1, 12, 11, 4, 9, 13, 10, 6, 7, 2};
    // std::array<unsigned char, 32> genome{4, 10, 3, 8, 250, 6, 7, 2, 18, 8, 9, 1, 5, 11, 9, 9, 9, 9, 9, 8, 10, 9, 1, 12, 11, 4, 9, 13, 10, 6, 7, 2};
    std::array<unsigned char, genomeSize> genome{4,2,17,8,250,5,7,3,5,18,9,1,5,11,9,9,9,9,9,6,2,9,11,11,11,11,9,9,9,9,1,9,16,0};
    std::array<unsigned char, memorySize> memory;
    unsigned x;
    unsigned y;
    QSharedPointer<Bot> saved;
    unsigned pointer = -1;
    unsigned way = Top;
    int energy = 100;
    unsigned minerals = 0;
    int type = 0;
    unsigned age = 0;
    unsigned lastSyntes;
    unsigned lastMinerals;

    std::array<unsigned short, 4> msgs{};
    unsigned last_msg_time = 0;

    unsigned last_under_attack = 0;

    bool alive = true;
    bool isSelected = false;
};

#endif // BOT_H
