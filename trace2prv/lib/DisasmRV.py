import re
from typing import List

from lib.paraver import *
from lib.ParaverLine import *


class DisasmRV:
    last_inst_without_rdest = True
    last_inst_with_addr = False
    last_inst_setvec = False
    last_inst_masked = False
    last_inst_overwrite_src = False

    last_inst_num_rsrc = 0
    last_reg_dst = 0

    # Number of consecutive instructions without destination register
    n_inst_without_rdest = 2

    current_inst_setvec = False
    current_inst_without_rdest = False
    current_rdest = 0
    current_inst_with_addr = False
    current_inst_masked = False
    current_inst_overwrite_src = False

    current_inst_num_rsrc = 0

    insts_without_rdest = ["vse", "vs1r"]
    insts_with_addr = ["vse", "vle", "vs1r", "vl1r"]
    insts_setvec = ["vsetli"]
    insts_overwrite_src = ["vmacc", "vnmsac", "vmadd", "vnmsub", "vwmaccu", "vwmacc", "vwmaccsu", "vwmaccus", "vqmaccu", "vqmacc", "vqmaccsu", "vqmaccus", "vfmacc", "vfnmacc", "vfmsac","vfnmsac","vfmadd","vfnmadd","vfmsub","vfnmsub","vfwmacc","vfwnmacc","vfwmsac","vfwnmsac"]


    rdest_index = [0]
    rsrc_index = [1, 2, 3]
    imm_index = [4, 5]
    vs_index = [6, 7, 8]
    inst_index = [9]
    raddr_index = [10]
    immaddr_index = [11]
    mask_index = [12]
    inst_overwrite_index = [13]
    vlen_index = [17]
    elen_index = [18]


    re_immediate = re.compile("(\s|,)\s*(0x)?([0-9]+)\s*(,|\(|$)")
    re_addr = re.compile("((0x)?[0-9]*)?\((.*)\)")
    re_inst = re.compile("^(\w*)(\.|\t|\n|\s|$)")
    re_mask = re.compile("(v[0-9]+)\.t")

    def __init__(self):
        pass

    def userEvents(self, row, paraverEvents, paraver_line):
        if row.startswith(".trace"):
            row = row[len(".trace"):];

            rdest_index = [0]

            row_split = row.split(",")
            usr_event = PrvEvent("", int(row_split[0]))

            paraver_line.addEvent(paraverEvents[self.inst_index[0]], 0)
            paraver_line.addEvent(usr_event, int(row_split[1]))
        elif row.startswith(".config "):
            row = row[len(".config "):];
            # Parse VLEN and ELEN
            row_split = row.split(",")
            assert len(row_split) == 2
            row_split = list(map(lambda x : x.split("="), row_split))
            assert row_split[0][0] == "VLEN"
            assert row_split[1][0] == "ELEN"
            paraver_line.addEvent(paraverEvents[self.vlen_index[0]], int(row_split[0][1]))
            paraver_line.addEvent(paraverEvents[self.elen_index[0]], int(row_split[1][1]))
        else:
            assert False

    def getInstructionArguments(self, disasm):

        arguments = disasm.split("\t")[1:]

        arguments = arguments[0].split(",")
        # arguments = [x for x in arguments if not x=='' ]
        arguments = [x[:-2] if x[-1] == ',' else x for x in arguments]

        return arguments

    def getValuesHashedFromDisasm(self, disasm, paraverEvents, paraverLine, start, end, add_new=False):
        position = start
        args = self.getInstructionArguments(disasm)

        for a in args:
            a = a.replace(" ", "")
            a = a.replace("(", "")
            a = a.replace(")", "")

            if not a in paraverEvents[position].values:
                if not add_new:
                    continue

                num_values = len(paraverEvents[position].values)
                paraverEvents[position].addValue(num_values + 1, a)

            paraverLine.addEvent(paraverEvents[position], paraverEvents[position].values[a])
            position = position + 1

        return position


    def getImmediateFromDisasm(self, disasm, paraverEvents, paraverLine):
        match = self.re_immediate.search(disasm)
        if match is not None:
            imm = match.group(3)
            paraverLine.addEvent(paraverEvents[4], imm)

    def getAddrFromDisasm(self, disasm, paraverevents: PrvEvent, paraverline: ParaverLine, raddr_index, imm_index):
        match = self.re_addr.search(disasm)
        if match is not None:
            imm = match.groups()[0]
            reg = match.groups()[2]
            if imm == "":
                imm = 0
            else:
                imm = int(imm, 0)
            paraverline.addEvent(paraverevents[self.raddr_index[0]], paraverevents[raddr_index].values[reg])
            paraverline.addEvent(paraverevents[self.imm_index[0]], imm)

    def getInstruction(self, disasm):
        return self.re_inst.search(disasm).group(1)

    def getMaskFromDisasm(self, disasm, paraver_event: PrvEvent, paraver_line: ParaverLine):

        match = self.re_mask.search(disasm)

        if match is not None:
            mask_reg = match.group(1)
            value = paraver_event.values[mask_reg]
            paraver_line.addEvent(paraver_event, value)
            return True
        else:
            return False

    def inference_null_values_setvec(self, paraverEvents, paraverLine):
        if not self.current_inst_setvec and self.last_inst_setvec:
            for i in self.vs_index:
                paraverLine.addEvent(paraverEvents[i], 0)
        self.last_inst_setvec = self.current_inst_setvec

    def inference_null_values_with_address(self, paraverEvents, paraverLine):
        if not self.current_inst_with_addr and self.last_inst_with_addr:
            for i in self.raddr_index + self.immaddr_index:
                paraverLine.addEvent(paraverEvents[i], 0)
        self.last_inst_with_addr = self.current_inst_with_addr

    def inference_null_values_with_mask(self, paraverEvents, paraverLine):
        if not self.current_inst_masked and self.last_inst_masked:
            paraverLine.addEvent(paraverEvents[self.mask_index[0]], 0)
        self.last_inst_masked = self.current_inst_masked

    def inference_null_values_rsrc(self, paraver_events: List[PrvEvent], paraver_line: ParaverLine):
        position = self.current_inst_num_rsrc
        end = self.last_inst_num_rsrc
        while position < end:
            position = position + 1
            paraver_line.addEvent(paraver_events[position], 0)
        self.last_inst_num_rsrc = self.current_inst_num_rsrc

    def inference_null_values_overwrite(self, paraver_events: List[PrvEvent], paraver_line: ParaverLine):
        if not self.current_inst_overwrite_src and self.last_inst_overwrite_src:
            paraver_line.addEvent(paraver_events[self.inst_overwrite_index[0]], 0)
        self.last_inst_overwrite_src = self.current_inst_overwrite_src

    def inference_null_values(self, paraver_events: List[PrvEvent], paraver_line: ParaverLine):
        self.inference_null_values_setvec(paraver_events, paraver_line)
        self.inference_null_values_with_address(paraver_events, paraver_line)
        self.inference_null_values_with_mask(paraver_events, paraver_line)
        self.inference_null_values_rsrc(paraver_events, paraver_line)
        self.inference_null_values_overwrite(paraver_events, paraver_line)

    def inference_rdest(self, paraverEvents, paraverLine):
        # If last instruction have a rdest put in actual instruction
        if self.n_inst_without_rdest == 0:
            paraverLine.addEvent(paraverEvents[self.rdest_index[0]], self.last_reg_dst)
        elif self.n_inst_without_rdest == 1:
            paraverLine.addEvent(paraverEvents[self.rdest_index[0]], 0)

        self.last_reg_dst = self.current_rdest
        # Count consecutive number of instructions without destine register
        if not self.current_inst_without_rdest:
            self.n_inst_without_rdest = 0
        else:
            self.n_inst_without_rdest = self.n_inst_without_rdest + 1


    def disasmParser(self, disasm, paraverEvents, paraverLine):
        inst = self.getInstruction(disasm)
        paraverLine.addEvent(paraverEvents[self.inst_index[0]], paraverEvents[self.inst_index[0]].values[inst])

        # Parse setvec instructions
        if inst in self.insts_setvec:
            self.getValuesHashedFromDisasm(disasm, paraverEvents, paraverLine, self.vs_index[0], self.vs_index[-1],
                                           False)
            self.current_inst_setvec = True
        else:
            self.current_inst_setvec = False

        # Parse instructions with addresses
        if inst in self.insts_with_addr:
             self.getAddrFromDisasm(disasm, paraverEvents, paraverLine, self.raddr_index[0], self.immaddr_index[0])
             self.current_inst_with_addr = True
        else:
            self.current_inst_with_addr = False

        # Parse instructions without destination register
        if inst in self.insts_without_rdest:
            self.current_inst_num_rsrc = self.getValuesHashedFromDisasm(disasm, paraverEvents, paraverLine, self.rsrc_index[0], self.rsrc_index[-1],
                                           False)
            self.current_rdest = 0
            self.current_inst_without_rdest = True
        else:
            self.current_inst_num_rsrc = self.getValuesHashedFromDisasm(disasm, paraverEvents, paraverLine, self.rdest_index[0], self.rsrc_index[-1],
                                                                          False)
            self.current_inst_num_rsrc = self.current_inst_num_rsrc - 1
            self.current_rdest = paraverLine.removeEvent(paraverEvents[self.rdest_index[0]])
            self.current_inst_without_rdest= False

        if inst in self.insts_overwrite_src:
            self.current_inst_overwrite_src = True
            paraverLine.addEvent(paraverEvents[self.inst_overwrite_index[0]], self.current_rdest)
        else:
            self.current_inst_overwrite_src = False

        self.current_inst_masked = self.getMaskFromDisasm(disasm, paraverEvents[self.mask_index[0]], paraverLine)
        self.getImmediateFromDisasm(disasm, paraverEvents, paraverLine)

    def neutral_state(self):
        self.current_rdest = 0
        self.current_inst_num_rsrc = 0
        self.current_inst_without_rdest = True
        self.current_inst_with_addr = False
        self.current_inst_setvec = False
        self.current_inst_masked = False
        self.current_inst_overwrite_src = False

    def disasmToPRV(self, disasm, paraver_events, paraver_line):
        if disasm[0] == '.':
            self.userEvents(disasm, paraver_events, paraver_line)
            self.neutral_state()
        else:
            self.disasmParser(disasm, paraver_events, paraver_line)

        self.inference_null_values(paraver_events, paraver_line)
        self.inference_rdest(paraver_events, paraver_line)
