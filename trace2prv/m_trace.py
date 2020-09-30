#!/usr/bin/env python3
import sys, re, csv, decimal
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


derived_event_dict= {
                        "resume" : 4,
                        "stall" : 5,
                        "l2_read" : 6,
                        "l2_write" : 7,
                        "l2_miss" : 8,
                        "request_to_bank" : 9
                    }


def offset_parser(offsets_string, paraver_events, paraver_line, last_inst_with_offsets):
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

def offset_parser_with_delay(offsets_string, paraver_events, paraver_line, last_state):
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
        tid = row[1]
        if tid not in thread_id:
             thread_id[tid] = accum
             accum = accum + 1

    return thread_id


def intToPRV(string, event, paraver_line, last_state):
    value = 0
    new_value=False

    if string != "":
        value = int(string, 0)
        new_value=True

    if last_state != 0 or new_value:
        paraver_line.addEvent(event, value)

    return value


def threadID(string, event, paraver_line, last_state):
    paraver_line.threadID = last_state[string] + 1
    return last_state


def coreID(string, event, paraver_line, last_state):
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

    paraver_line = ParaverLine(2, 1, 1, 1, 1)

    last_dst_value = 0
    last_dst_event = PrvEvents[2].derivedEvents[0]

    #disasm_col = 2
    offset_col = 0
    coreid_col = 1
    event_type_col = 2
    #coreid_col = 10

    NUMBER_OF_COLUMS = 4

    last_state = [0] * NUMBER_OF_COLUMS
    parser_functions = [intToPRV] * NUMBER_OF_COLUMS


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

    for row in data:

        if (sanityCheck(row, True) == -1): continue

        paraver_line.time = row[0]
        paraver_line.events = []

        i = 0

        for col in row[1:-1]:
            i = i + 1
            if i==2: #If this is the event type, pass the value, which is in the last element of the row
                ev=str(PrvEvents[i].derivedEvents[derived_event_dict[col]-len(derived_event_dict)+2].id)
                last_state[i] = parser_functions[i](row[-1], PrvEvents[i].derivedEvents[derived_event_dict[col]-len(derived_event_dict)+2], paraver_line, last_state[i])
            else:
                last_state[i] = parser_functions[i](col, PrvEvents[i], paraver_line, last_state[i])
               

        writePRVFile(prvfile, paraver_line.getLine())

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

    parser.add_argument("input", help="Vehave trace file")
    parser.add_argument("--output-dir", default=".", help="Path to output directory. If not specified will be cwd")
    parser.add_argument("--output-name", help="Paraver trace name. If not specified will have the same basename than the input")
    parser.add_argument("--events-sorted", action='store_true', help="Paraver events sorted")
    parser.add_argument("--time-between-instruction", type=int, default=1, help="Time in nanoseconds between instructions")
    parser.add_argument("--offset-events-with-delay", action='store_true', help="The Offset events will be separated by 1 nanosecond")
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

    print(eventsNames)

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
    createRow(args.output_name, args.output_dir, "spike_sparta", nthreads)
    createPcf(args.output_name, args.output_dir, prvEvents)
    end = time.time()

    post_time = round(end - start, 2)
    printMsg("Processing time: " + str(post_time) + " seconds", "INFO")


if __name__ == "__main__":
    main()