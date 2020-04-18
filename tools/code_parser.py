codes = list(map(int, input().split()))

kek = [0] * len(codes)

names = [
    "Lazy",           #0
    "Forward",        #1
    "Left",           #2
    "Right",          #3
    "Check",          #4
    "RelJmp",         #5
    "Eat",            #6
    "NewBot",         #7
    "CheckEnergy",    #8
    "Syntes",         #9
    "GiveEnergy",     #10
    "Minerals",       #11
    "GiveMinerals",   #12
    "CheckAlive",     #13
    "SendMsg",        #14
    "CheckMsg",       #15
    "AbsJmp",         #16
    "CheckAge",       #17
    "CheckMinerals",  #18
    "StoreAbs",       #19
    "StoreRel",       #20
    "CheckStoreAbs",  #21
    "CheckStoreRel",  #22
    "IncStoreAbs",    #23
    "IncStoreRel",    #24
    "CopyStoreAbs",   #25
    "CopyStoreRel",   #26
    "MsgStoreAbs",    #27
    "MsgStore",       #28
    "CheckWay",       #29
    "SaveGenome",     #30
    "CheckSaved",     #31
    "ResetSaved",     #32
    "ClearMessage",   #33
    "CheckLastSyn",   #34
    "CheckLastMin",   #35
]


comands_num = len(names)


class Info:
    command = None
    meta = None
    ifyes = None
    ifnone = None

    def __init__(self, pointer, genome):
        pointer %= len(genome)
        self.pointer = pointer
        if kek[pointer] != 0:
            self.command = -1
            self.to = pointer
            return
        kek[pointer] = self
        self.command = genome[pointer] % comands_num
        if self.command in [0, 1, 2, 3, 6, 9, 11, 30, 32, 33]:
            self.next = Info(pointer + 1, genome)
        if self.command in [7, 10, 12, 14, 27, 28, 29, 31, 34, 35]:
            self.meta = genome[(pointer + 1) % len(genome)]
            self.next = Info(pointer + 2, genome)
        elif self.command == 4:
            self.ifnone = Info(pointer + 1 + genome[(pointer + 1) % len(genome)], genome)
            self.ifsiml = Info(pointer + 1 + genome[(pointer + 2) % len(genome)], genome)
            self.ifelse = Info(pointer + 1 + genome[(pointer + 3) % len(genome)], genome)
        elif self.command == 5:
            self.next = Info(pointer + 1 + genome[(pointer + 1) % len(genome)], genome)
        elif self.command in [8, 15, 17, 18, 24, 29]:
            self.meta = genome[(pointer + 1) % len(genome)]
            self.ifyes = Info(pointer + 3, genome)
            self.ifnot = Info(pointer + 1 + genome[(pointer + 2) % len(genome)], genome)
        elif self.command == 13:
            self.ifyes = Info(pointer + 2, genome)
            self.ifnot = Info(pointer + 1 + genome[(pointer + 1) % len(genome)], genome)
        elif self.command == 16:
            self.next = Info(genome[(pointer + 1) % len(genome)], genome)
        elif self.command in [19, 20, 23, 24, 25, 26]:
            self.meta = ((pointer + 1) % len(genome), (pointer + 2) % len(genome))
            self.next = Info(pointer + 1 + 2, genome)
        elif self.command in [21, 22]:
            self.meta = ((pointer + 1) % len(genome), (pointer + 2) % len(genome))
            self.ifyes = Info(pointer + 3, genome)
            self.ifnot = Info(pointer + 1 + genome[(pointer + 2) % len(genome)], genome)

    def str(self, req=0):
        if self.command == -1:
            if req < 0:
                return kek[self.to].str(req)
            return str(self.to)
        s = names[self.command] + "(" + str(self.pointer) + ")"
        if self.meta is not None:
            s += " " + str(self.meta)

        if self.ifnone is not None:
            s += "\n"
            s += "|   " * req
            s += "None: " + self.ifnone.str(req + 1)
            s += "\n"
            s += "|   " * req
            s += "Sibl: " + self.ifsiml.str(req + 1)
            s += "\n"
            s += "|   " * req
            s += "Else: " + self.ifelse.str(req + 1)
        elif self.ifyes is not None:
            s += "\n"
            s += "|   " * req
            s += "Yes: " + self.ifyes.str(req + 1)
            s += "\n"
            s += "|   " * req
            s += "No:  " + self.ifnot.str(req + 1)
        else:
            s += " -> " + self.next.str(req)

        return s

    def __str__(self):
        return self.str()


print(' '.join(map(str, codes)))
print(Info(0, codes))
