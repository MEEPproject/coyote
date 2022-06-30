#!/usr/bin/env python3
import sys, re, csv, decimal, operator
import time
import argparse
from lib.utils import *
from lib.paraver import *
from lib.events import *
from lib.genericCSVtoPRV import *
from lib.asm2prv import *
from lib.ParaverLine import *
import lib.ParaverLine
from lib.DisasmRV import *
import subprocess

base_event_dict= {
                    "pc" : 0,
                    "l1_miss" : 1,
                    "l1_hit" : 1,
                    "l1_address" : 2,
                    "local_request" : 3,
                    "remote_request" : 4,
                    "l1_bypass" : 2,
                    "l2_address" : 5,
                    "memory_request" : 6,
                    "memory_operation" : 7,
                    "memory_ack" : 8,
                    "ack_received" : 9,
                    "ack_forwarded" : 10,
                    "ack_forward_received" : 11,
                    "miss_serviced" : 12,
                    "stall" : 13,
                    "resume" : 14,
                    "KI" : 15,
                    "bank_operation" : 16,
                    "miss_on_evicted" : 17,
                    "resume_address" : 18,
                    "resume_mc" : 19,
                    "resume_memory_bank" : 20,
                    "resume_cache_bank" : 21,
                    "resume_tile" : 22,
                    "noc_message_dst" : 23,
                    "noc_message_src" : 24,
                    "noc_message_dst_cummulated" : 25,
                    "noc_message_src_cummulated" : 26,
                    "mem_tile_occupancy_out_noc"    : 27,
                    "mem_tile_occupancy_mc"         : 28,
                    "mem_tile_vvl"                  : 29,
                    "mem_tile_vecop_recv"           : 30,
                    "mem_tile_vecop_sent"           : 31,
                    "mem_tile_scaop_recv"           : 32,
                    "mem_tile_scaop_sent"           : 33,
                    "mem_tile_spop_recv"            : 34,
                    "mem_tile_spop_sent"            : 35,
                    "mem_tile_mtop_recv"            : 36,
                    "mem_tile_mtop_sent"            : 37,
                    "mem_tile_mc_recv"              : 38,
                    "mem_tile_mc_sent"              : 39,
                    "mem_tile_llc_recv"             : 40,
                    "mem_tile_llc_sent"             : 41,
                    "mem_tile_mc2llc"               : 42,
                    "mem_tile_llc2mc"               : 43,
                    "mem_tile_noc_recv"             : 44,
                    "mem_tile_noc_sent"             : 45,
                    "inst" : 46,
                    "l2_read" : 47,
                    "l2_write" : 48,
                    "memory_read" : 49,
                    "memory_write" : 50,
                    "set_vl" : 51,
                    "requested_vl" : 52,
                    "l2_wb" : 53,
                    "l2_hit" : 54,
                    "l2_miss" : 54,
                    "InstructionOperand1" : 55,
                    "InstructionOperand2" : 56,
                    "InstructionOperand3" : 57
                };
                #MemoryObj events should remain the last in the json file!!

def offset_parser(offsets_string, paraver_events, paraver_line, last_inst_with_offsets, PrvEvents):
    counter = 0
    offsets_strip = offsets_string.strip()
    events = paraver_events.derivedEvents

    if offsets_strip:
        last_inst_with_offsets = True
        offsets = offsets_string.split(',')
        paraver_line.addEvent(paraver_events, 1)
        for offset in offsets:
            value = int(offset, 0)
            paraver_line.addEvent(events[counter], value)
            counter = counter + 1
    elif last_inst_with_offsets:
        last_inst_with_offsets = False
        paraver_line.addEvent(paraver_events, 0)
         
    return last_inst_with_offsets

def offset_parser_with_delay(offsets_string, paraver_events, paraver_line, last_state, PrvEvents):
    #Last_state:
    #last_state[0] int with time between instructions
    #last_state[1] prv file

    time_between_instructions = last_state[0]
    prvfile = last_state[1]

    counter = 0

    offsets_strip = offsets_string.strip()
    events = paraver_events.derivedEvents

    if offsets_strip:
        offsets = offsets_string.split(',')
        for offset in offsets:
            offset_event_time = str(int(paraver_line.time) + counter)
            value = int(offset, 0)
            offset_paraver_line = ParaverLine(2, paraver_line.cpuId, paraver_line.applId, paraver_line.taskId, paraver_line.threadId, offset_event_time)
            offset_paraver_line.addEvent(events[0], value)
            #if offset_event_time != paraver_line.time:
            #    offset_paraver_line.addEvent(events[counter-1], 0)
            writePRVFile(prvfile, offset_paraver_line.getLine())
            counter = counter + 1

    offset_event_time = str(int(paraver_line.time) + counter)
    offset_paraver_line = ParaverLine(2, paraver_line.cpuId, paraver_line.applId, paraver_line.taskId, paraver_line.threadId, offset_event_time)
    offset_paraver_line.addEvent(events[0], 0)

    assert counter < time_between_instructions, "The time between instructions has to be greater than the number of offsets."
    writePRVFile(prvfile, offset_paraver_line.getLine())


    return last_state


def getThreadsFromTrace(csvfile):
    data = csv.reader(csvfile, skipinitialspace=True, delimiter=',')

    line = next(data)
    thread_id = dict()

    accum = 0
    for row in data:
        #print(row)
        tid = row[1]
        if tid not in thread_id:
             thread_id[tid] = accum
             accum = accum + 1

    return thread_id

def instToTrace(string, event, paraver_line, last_state):
    paraver_line.addEvent(event, string)


def intToPRV(string, event, paraver_line, last_state, PrvEvents):
    value = 0 
    new_value=False
    if string != "":
        base=0
        #if event.name=="pc" or "Request" in event.name or event.name=="address" or event.name=="MemoryOperation" or "Ack" in event.name or event.name=="L1MissServiced" or event.name=="Resume" or event.name=="BankOperation":
        #print("\t"+string)
        if event.name in ["PC", "BaseAddressAccessedByInstruction", "MemoryOperation", "MemoryRead", "MemoryWrite", "L1Hit", "L1Miss", "L2Hit", "L2Miss", "Ack", "L1MissServiced", "ResumeAddress", "BankOperation", "L2Read", "L2Write", "L2Writeback"] or ("Request" in event.name and "Requested" not in event.name) or "Ack" in event.name or "MemTile" in event.name:
            base=16
        value=int(string, base)
        #print("\t\t"+str(value))
        #value=string
        new_value=True

    if last_state != 0 or new_value:
        paraver_line.addEvent(event, value)

    return value



def threadID(string, event, paraver_line, last_state):
    paraver_line.threadID = last_state[string] + 1
    return last_state


def coreID(string, event, paraver_line, last_state, PrvEvents):
    paraver_line.threadId = int(string)+1
    return  last_state


def interfaceDisasm(disasm, paraver_events, paraver_line, last_state):
    last_state.disasmToPRV(disasm, paraver_events.derivedEvents, paraver_line)
    return last_state


def spikeSpartaTraceToPrv(csvfile, prvfile, PrvEvents, threads, args):
   

    nThreads = len(threads)
    header = paraverHeader(csvfile, nthreads=nThreads, delimiter=",")
    writePRVFile(prvfile, header)

    csvfile.seek(0)
    data = csv.reader(csvfile, skipinitialspace=True, delimiter=',')

    next(data,None) #Skip header
    data = sorted(data, key=lambda x:int(x[0])) 

    paraver_line = ParaverLine(2, 1, 1, 1, 1)
   

    last_dst_value = 0

    #disasm_col = 2
    offset_col = 0
    coreid_col = 1
    event_type_col = 3
    #coreid_col = 10
    
    pc_col=2
    address_col=5


    NUMBER_OF_COLUMS = 6

    last_state = [0] * NUMBER_OF_COLUMS
    parser_functions = [intToPRV] * NUMBER_OF_COLUMS

    mem_obj_name_pos=-3
    mem_obj_start_pos=-2
    mem_obj_end_pos=-1
    mem_objs=[]

    #for a in PrvEvents:
        #print(a)

    if args.exe!=None:
        command = 'riscv64-unknown-elf-nm --print-size --size-sort '+args.exe
        process = subprocess.Popen(command.split(), stdout=subprocess.PIPE)
        output, error = process.communicate()
        PrvEvents[mem_obj_name_pos].addValue(0, "None")
        mem_objs.append((-1, -1))
        i=1
        for l in output.splitlines():
            tokens=l.decode("utf-8").split(' ')
            start=int(tokens[0], 16)
            end=start+int(tokens[1],16)
            name=tokens[3]

            paraver_line.time = 0
            paraver_line.events = []
            paraver_line.addEvent(PrvEvents[mem_obj_name_pos], i)
            PrvEvents[mem_obj_name_pos].addValue(i, name)

            paraver_line.addEvent(PrvEvents[mem_obj_start_pos], start)
            paraver_line.addEvent(PrvEvents[mem_obj_end_pos], end)

            mem_objs.append((start, end))

            writePRVFile(prvfile, paraver_line.getLine())

            i=i+1

        PrvEvents[mem_obj_name_pos].addValue(len(mem_objs), "Other")

    if args.offset_events_with_delay:
        last_state[offset_col] = [args.time_between_instruction, prvfile]
        parser_functions[offset_col] = offset_parser_with_delay
    else:
        last_state[offset_col] = False
        parser_functions[offset_col] = offset_parser

    parser_functions[coreid_col] = coreID

    last_state[coreid_col] = threads
    parser_functions[coreid_col] = coreID
    #parser_functions[event_type_col] = eventTypeParser

    #sew_index = 11
    #lmul_index = 12
    
    prev_type=""
    prev_time=0
    prev_core="-1"
    num_different_instructions=0
    num_operands1=0;
    num_operands2=0;
    num_operands3=0;
                    
    num_stall_reasons=0
    key=num_stall_reasons
    PrvEvents[base_event_dict["stall"]].addValue(key, "resume")
    num_stall_reasons=num_stall_reasons+1
            
    prev_had_l1_miss=False
    prev_had_l2_miss=False
    has_l1_miss=False
    has_l2_miss=False

    for row in data:

        if (sanityCheck(row, True) == -1): continue

        if int(row[0])>prev_time:
            if int(row[0])==prev_time+1:
                if prev_had_l1_miss and not has_l1_miss:
                    paraver_line.addEvent(PrvEvents[base_event_dict["l1_miss"]], 0)
                if prev_had_l2_miss and not has_l2_miss:
                    paraver_line.addEvent(PrvEvents[base_event_dict["l2_miss"]], 0)

            writePRVFile(prvfile, paraver_line.getLine())
            paraver_line.time = row[0]
            paraver_line.events = []

            #Add 0 after cache misses
            if int(row[0])>prev_time+1 and (has_l1_miss or has_l2_miss):
                aux_paraver_line = ParaverLine(2, 1, 1, 1, 1)
                aux_paraver_line.time = prev_time+1
                paraver_line.events = []
                if has_l1_miss:
                    aux_paraver_line.addEvent(PrvEvents[base_event_dict["l1_miss"]], 0)

                if  has_l2_miss:
                    aux_paraver_line.addEvent(PrvEvents[base_event_dict["l2_miss"]], 0)

                writePRVFile(prvfile, aux_paraver_line.getLine())
                has_l1_miss=False
                has_l2_miss=False

            prev_had_l1_miss=has_l1_miss
            prev_had_l2_miss=has_l2_miss
            has_l1_miss=False
            has_l2_miss=False

        #print("["+row[0]+"]")

        #To be able to count events, we need to add an event with value zero after an actual event of the kind to count (unless there is an event of that kind actually happening the cycle after)
    #    if prev_type in ["l2_miss", "KI", "bank_operation", "miss_on_evicted", "resume_cache_bank"] and (row[coreid_col]!=prev_core or prev_type!=row[3] or ( prev_type==row[3] and  int(paraver_line.time)>prev_time+1)):
 #           last_state[3] = parser_functions[3]("0", PrvEvents[base_event_dict[prev_type]], paraver_line, last_state[3], PrvEvents)
  #          paraver_line.time = str(prev_time+1)
   #         writePRVFile(prvfile, paraver_line.getLine())
    #        paraver_line.time = row[0]
     #       paraver_line.events = []

        i = 0 #Starts at 1 because the firs column after the time stamp is not an event

        for col in row[1:]:
            i = i + 1
            if row[3]=="inst_graduate":
                break

            if (i==2 or i==4 or i==5) and ("resume" in row[3] or row[3]=="stall" or row[3]=="KI"): #These events have no associated pc, address or destination
                continue

            if i==4: # and (row[3] in ["l2_miss", "bank_operation", "inst"]): #L2 misses have no destination and instructions
                continue

            if i==5: # and (row[3] in ['local_request', 'remote_request', 'surrogate_request', 'memory_request', 'memory_operation', 'memory_ack', 'ack_received', 'ack_forward_received', "ack_forwarded", "miss_serviced", "mem_tile_mc_recv", "mem_tile_mc_sent", "mem_tile_occupancy_out_noc", "mem_tile_occupancy_mc", "mem_tile_vvl", "mem_tile_vecop_recv", "mem_tile_vecop_sent", "mem_tile_scaop_recv", "mem_tile_scaop_sent", "mem_tile_spop_recv", "mem_tile_spop_sent", "mem_tile_mtop_recv", "mem_tile_mtop_sent", "mem_tile_noc_recv", "mem_tile_noc_sent"]): #These events already have their address as the semantic value
                continue

            if i==3: #If this is the event type, pass the value, which is in the last element of the row
                value=row[-1]
                if col=="l2_miss" or col=="KI":
                    value="1"
                elif col=="bank_operation":
                    value=str(int(row[4])+1) #Banks should start from 1 so we can distinguish the case in which no bank was accessed
                elif col=="miss_on_evicted":
                    value=row[4]
                elif col=="inst":
                    value=row[4]

                #print("col is "+col)
                if col=="l1_miss" or col=="l1_hit":
                    miss=0
                    if col=="l1_miss":
                        miss=1
                    paraver_line.addEvent(PrvEvents[base_event_dict["l1_miss"]], miss)
                    paraver_line.addEvent(PrvEvents[base_event_dict["l1_address"]], int(str(value), 16))
                elif col=="l2_miss" or col=="l2_hit":
                    miss=0
                    if col=="l2_miss":
                        miss=1
                    paraver_line.addEvent(PrvEvents[base_event_dict["l2_miss"]], miss)
                    paraver_line.addEvent(PrvEvents[base_event_dict["l2_address"]], int(str(value), 16))

                elif col=="inst":
                    #print(str(derived_event_dict[col])+" ->  "+value)
                    #print(PrvEvents[base_event_dict[i]].derivedEvents[derived_event_dict[col]-2])
                    key=-1

                    tokens=value.split()
                    opcode=tokens[0]
                    operand1=""
                    operand2=""
                    operand3=""
                    if len(tokens)>1:
                        match=re.match("[a-z]\d+",tokens[1])
                        if match!=None:
                            operand1=match.group(0)
                        if len(tokens)>2:
                            match=re.match("[a-z]\d+",tokens[2])
                            if match!=None:
                                operand2=match.group(0)
                            if len(tokens)>3:
                                match=re.match("[a-z]\d+",tokens[3])
                                if match!=None:
                                    operand3=match.group(0)


                    if not PrvEvents[base_event_dict[col]].containsKey(opcode):
                        key=num_different_instructions
                        PrvEvents[base_event_dict[col]].addValue(key, opcode)
                        num_different_instructions=num_different_instructions+1
                    else:
                        key=PrvEvents[base_event_dict[col]].getValue(opcode)
                    last_state[i] = parser_functions[i](str(key), PrvEvents[base_event_dict[col]], paraver_line, last_state[i], PrvEvents)
                    
                    if operand1!="":
                        if not PrvEvents[base_event_dict["InstructionOperand1"]].containsKey(operand1):
                            key=num_operands1
                            PrvEvents[base_event_dict["InstructionOperand1"]].addValue(key, operand1)
                            num_operands1=num_operands1+1
                        else:
                            key=PrvEvents[base_event_dict["InstructionOperand1"]].getValue(operand1)
                        last_state[i] = parser_functions[i](str(key), PrvEvents[base_event_dict["InstructionOperand1"]], paraver_line, last_state[i], PrvEvents)
                    
                    if operand2!="":
                        if not PrvEvents[base_event_dict["InstructionOperand2"]].containsKey(operand2):
                            key=num_operands2
                            PrvEvents[base_event_dict["InstructionOperand2"]].addValue(key, operand2)
                            num_operands2=num_operands2+1
                        else:
                            key=PrvEvents[base_event_dict["InstructionOperand2"]].getValue(operand2)
                        last_state[i] = parser_functions[i](str(key), PrvEvents[base_event_dict["InstructionOperand2"]], paraver_line, last_state[i], PrvEvents)
                    
                    if operand3!="":
                        if not PrvEvents[base_event_dict["InstructionOperand3"]].containsKey(operand3):
                            key=num_operands3
                            PrvEvents[base_event_dict["InstructionOperand3"]].addValue(key, operand3)
                            num_operands3=num_operands3+1
                        else:
                            key=PrvEvents[base_event_dict["InstructionOperand3"]].getValue(operand3)
                        last_state[i] = parser_functions[i](str(key), PrvEvents[base_event_dict["InstructionOperand3"]], paraver_line, last_state[i], PrvEvents)
                    
                    last_state[i] = parser_functions[i](row[2], PrvEvents[base_event_dict["pc"]], paraver_line, last_state[i], PrvEvents)

                elif col=="stall" or col=="resume": #stall
                    key=-1
                    if col=="resume":
                        value="resume"

                    if value=="raw":
                        value="raw/resource unavailable"

                    if not PrvEvents[base_event_dict["stall"]].containsKey(value):
                        key=num_stall_reasons
                        PrvEvents[base_event_dict["stall"]].addValue(key, value)
                        num_stall_reasons=num_stall_reasons+1
                    else:
                        key=PrvEvents[base_event_dict["stall"]].getValue(value)
                        #print("NOPE")

                    last_state[i] = parser_functions[i](str(key), PrvEvents[base_event_dict["stall"]], paraver_line, last_state[i], PrvEvents)
                else:
                    #if derived_event_dict[col]-2==46:
                        #print("!!!!!!!!!!!"+col)
                    #print(col+" -> "+str(derived_event_dict[col]))
                    #print(len(PrvEvents[base_event_dict[i]].derivedEvents))
                    #print("value is "+value)
                    #print("id is "+str(derived_event_dict[col]-2))
                    last_state[i] = parser_functions[i](value, PrvEvents[base_event_dict[col]], paraver_line, last_state[i], PrvEvents)
                    if(col=="set_vl"):
                        last_state[i] = parser_functions[i](row[4], PrvEvents[base_event_dict["requested_vl"]], paraver_line, last_state[i], PrvEvents)

            else:
                if i==1: #Core id
                    parser_functions[i](col, None, paraver_line, last_state[i], PrvEvents)
                #else:
                    #if i==4 or i==5:
                     #   continue
                    #else:
                    #print(col)
                    #print(i)
                    #print(base_event_dict[i])
                    #last_state[i] = parser_functions[i](col, PrvEvents[base_event_dict[i]], paraver_line, last_state[i])

        if (args.exe!=None and row[3]!="resume" and row[3]!="stall" and row[3]!="KI") or (row[3]=="resume" and int(row[5], 16)!=0): #Add memory obj info. stalls, KIs and resumes with value 0 have no mem info
            found=False
            for i in range(len(mem_objs)):
                start, end=mem_objs[i]
                if start <= int(row[5], 16) < end:
                    paraver_line.addEvent(PrvEvents[mem_obj_name_pos], i)
                    found=True
                    break


            if not found:
                paraver_line.addEvent(PrvEvents[mem_obj_name_pos],len(mem_objs))
                    

        if row[3]=="l1_miss":
            has_l1_miss=True
        elif row[3]=="l2_miss":
            has_l2_miss=True

        prev_type=row[3]
        prev_time=int(row[0])
        prev_core=row[coreid_col]

    printMsg("PRV generated successfully!", "OK")


def generateOffsetEvents(num_events, start_ID):
    offset_event = PrvEvent("Valid-addres-VL", start_ID)

    counter = 0

    for i in range(start_ID + 1, start_ID + num_events + 1):
        name = "addres-VL-" + str(counter)
        tmp_event = PrvEvent(name, i)
        counter = counter + 1
        offset_event.derivedEvents.append(tmp_event)

    return offset_event

def parseArguments():

    parser = argparse.ArgumentParser()

    parser.add_argument("input", help="Behave trace file")
    parser.add_argument("--output-dir", default=".", help="Path to output directory. If not specified will be cwd")
    parser.add_argument("--output-name", help="Paraver trace name. If not specified will have the same basename than the input")
    parser.add_argument("--events-sorted", action='store_true', help="Paraver events sorted")
    parser.add_argument("--time-between-instruction", type=int, default=1, help="Time in nanoseconds between instructions")
    parser.add_argument("--offset-events-with-delay", action='store_true', help="The Offset events will be separated by 1 nanosecond")
    parser.add_argument("--exe", help="Path to the exe for static variable analysis.")
    args = parser.parse_args()

    ParaverLine.GET_LINE_SORTED = args.events_sorted

    return args

def main():
    args = parseArguments()

    filename = ntpath.basename(args.input)

    if args.output_name is None:
        args.output_name=filename

    start = time.time()
    csvfile = openCSVFile(args.input)
    prvfile = openPRVFile(args.output_name, args.output_dir, compress=False)

    eventsNames = readGenericCSVHeader(csvfile, ",")

    #print(eventsNames)

    prvEvents = getStdEventsFromJsonFile(eventsNames, "spike_sparta")

    scriptpath = os.path.dirname(os.path.realpath(__file__))

   # getRegistersValuesFromJson(scriptpath + "/lib/RISCV_ISA.json", prvEvents[2].derivedEvents[0])
   # getRegistersValuesFromJson(scriptpath + "/lib/RISCV_ISA.json", prvEvents[2].derivedEvents[1])
   # getRegistersValuesFromJson(scriptpath + "/lib/RISCV_ISA.json", prvEvents[2].derivedEvents[2])
   # getRegistersValuesFromJson(scriptpath + "/lib/RISCV_ISA.json", prvEvents[2].derivedEvents[3])
   # getRegistersValuesFromJson(scriptpath + "/lib/RISCV_ISA.json", prvEvents[2].derivedEvents[10])
   # getRegistersValuesFromJson(scriptpath + "/lib/RISCV_ISA.json", prvEvents[2].derivedEvents[12])
   # assignValuesFromJson(scriptpath + "/lib/RISCV_ISA.json", "Vtype", prvEvents[2].derivedEvents[6])
   # assignValuesFromJson(scriptpath + "/lib/RISCV_ISA.json", "Vtype", prvEvents[2].derivedEvents[7])
   # assignValuesFromJson(scriptpath + "/lib/RISCV_ISA.json", "Instructions", prvEvents[2].derivedEvents[9])

    #prvEvents[8] = generateOffsetEvents(2048, 48000000)

    thread_ids = getThreadsFromTrace(csvfile)

    nthreads = len(thread_ids)
    printMsg("Number of threads in trace is {}".format(nthreads), "INFO")

    end = time.time()

    pre_time = round(end - start, 2)
    printMsg("Preprocessing time: " + str(pre_time) + " seconds", "INFO")

    start = time.time()

    spikeSpartaTraceToPrv(csvfile, prvfile, prvEvents, thread_ids, args)
    createRow(args.output_name, args.output_dir, "core", nthreads)
    createPcf(args.output_name, args.output_dir, prvEvents)
    end = time.time()

    post_time = round(end - start, 2)
    printMsg("Processing time: " + str(post_time) + " seconds", "INFO")


if __name__ == "__main__":
    main()
