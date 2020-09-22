

def eventIntegerParser(text, args):
    return str(int(text,0))


def eventFloatParser(text, args):
    return str(int(ffloat(text)*args[0]))

def vehaveDisasmParser(text, args):
    RegistersPo


def getValuesHashedFromDisasm(disasm, paraverEvents, paraverLine, start, end):
    position = start
    args = getInstructionArguments(disasm)

    for a in args:
        a = a.replace(" ", "")

        if not a in paraverEvents[position].values:
            num_values = len(paraverEvents[position].values)
            paraverEvents[position].addValue(num_values + 1, a)

        paraverLine.addEvent(paraverEvents[position], paraverEvents[position].values[a])
        position = position + 1

    while position <= end:
        paraverLine.addEvent(paraverEvents[position], 0)
        position = position + 1


def getImmediateFromDisasm(disasm, paraverEvents, paraverLine):
    match = re.search(" (0x)?([0-9]+)(,|\(|$)", disasm)
    if match != None:
        imm = match.group(0)
        imm = imm.replace(" ", "")
        paraverLine.addEvent(paraverEvents[4], imm)


def disasmToString(disasm, paraverEvents, paraverLine):
    inst = getInstruction(disasm)

    paraverLine.addEvent(paraverEvents[9], paraverEvents[9].values[inst])

    if inst == "vsetvli":
        getValuesHashedFromDisasm(disasm, paraverEvents, paraverLine, 6, 8)

    getValuesHashedFromDisasm(disasm, paraverEvents, paraverLine, 0, 3)
    getImmediateFromDisasm(disasm, paraverEvents, paraverLine)

    # return line