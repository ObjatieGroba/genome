command_names = [
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
]

commands = {command_names[i]: i for i in range(len(command_names))}

var_type_names = [
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
]

var_types = {var_type_names[i]: i for i in range(len(var_type_names))}


codes = list(map(int, input().split()))

parsed_ptr = [None] * len(codes)


class Var:
    type = None
    pointer = None
    psize = None
    value = None
    meta = None

    def __init__(self, pointer, genome):
        pointer %= len(genome)
        self.pointer = pointer
        self.type = var_type_names[genome[pointer] % len(var_type_names)]
        if self.type == "ByteConstant":
            pointer += 1
            pointer %= len(genome)
            self.value = genome[pointer]
            self.psize = 2
        elif self.type == "MemAbs":
            pointer += 1
            pointer %= len(genome)
            self.meta = genome[pointer]
            self.psize = 2
            self.value = genome[self.meta % len(genome)]
        elif self.type == "MemRel":
            pointer += 1
            pointer %= len(genome)
            self.meta = genome[pointer]
            self.psize = 2
            self.value = genome[pointer + 1 + self.meta % len(genome)]
        elif self.type == "UIntConstant":
            self.value = 256 * 256 * 256 * genome[(pointer + 1) % len(genome)] + \
                        256 * 256 * genome[(pointer + 1) % len(genome)] + \
                        256 * genome[(pointer + 1) % len(genome)] + \
                        genome[(pointer + 1) % len(genome)]
            self.psize = 5
        else:
            self.psize = 1

    def __str__(self):
        s = self.type
        if self.meta is not None:
            s += '(' + str(self.meta) + ')'
        if self.value is not None:
            s += ' '
            s += str(self.value)
        return s


class RuntimeDefined:
    def str(self, req=0):
        return "RuntimeDefined"


class Info:
    command = None
    next = None
    next_yes = None
    next_no = None
    vars = []
    pointer = None

    def __init__(self, pointer, genome):
        pointer %= len(genome)
        self.pointer = pointer
        if parsed_ptr[pointer] is not None:
            self.command = -1
            self.to = pointer
            return
        parsed_ptr[pointer] = self
        self.command = command_names[genome[pointer] % len(command_names)]
        pointer += 1
        pointer %= len(genome)
        if self.command in ["Lazy", "Forward", "Left", "Right", "Synthesise", "MakeMinerals", "EatFront"]:
            self.next = Info(pointer, genome)
        elif self.command in ["GiveEnergy", "GiveMinerals", "SendMsg", "MakeNewBot"]:
            var = Var(pointer, genome)
            pointer += var.psize
            self.vars = [var]
            self.next = Info(pointer, genome)
        elif self.command == "RelativeJump":
            jump = Var(pointer, genome)
            pointer += jump.psize
            self.vars = [jump]
            if jump.value is not None:
                self.next = Info(pointer + jump.value, genome)
            else:
                self.next = RuntimeDefined()
        elif self.command == "AbsoluteJump":
            jump = Var(pointer, genome)
            pointer += jump.psize
            self.vars = [jump]
            if jump.value is not None:
                self.next = Info(jump.value, genome)
            else:
                self.next = RuntimeDefined()
        elif self.command in ["IfGEq", "IfEq", "IfLe"]:
            var1 = Var(pointer, genome)
            pointer += var1.psize
            var2 = Var(pointer, genome)
            pointer += var2.psize
            absjump = Var(pointer, genome)
            pointer += absjump.psize
            self.vars = [var1, var2, absjump]
            self.next_no = Info(pointer, genome)
            if absjump.value is not None:
                # Jump can be runtime defined
                self.next_yes = Info(absjump.value, genome)
            else:
                self.next_yes = RuntimeDefined()
        elif self.command in ["Store", "IncStore", "CopyStore", "StoreAbs", "IncStoreAbs", "CopyStoreAbs"]:
            var1 = Var(pointer, genome)
            pointer += var1.psize
            var2 = Var(pointer, genome)
            pointer += var2.psize
            self.vars = [var1, var2]
            self.next = Info(pointer, genome)
        else:
            raise RuntimeError("Unknown command " + self.command)

    def str(self, req=0):
        if self.command == -1:
            if req < 0:
                return parsed_ptr[self.to].str(req)
            return str(self.to)
        s = self.command + "(" + str(self.pointer) + ") "
        s += ' '.join(map(str, self.vars)) + ' '
        if self.command in ["IfGEq", "IfEq", "IfLe"]:
            s += "\n"
            s += "|   " * req
            s += "Yes: " + self.next_yes.str(req + 1)
            s += "\n"
            s += "|   " * req
            s += "No:  " + self.next_no.str(req + 1)
        else:
            s += "-> " + self.next.str(req)

        return s

    def __str__(self):
        return self.str()


print(' '.join(map(str, codes)))
print(Info(0, codes))
