#!/usr/bin/python3
# 
# Copyright 2022 Barcelona Supercomputing Center - Centro Nacional de
#                Supercomputación
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#     http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
# implied.
# See the LICENSE file in the root directory of the project for the
# specific language governing permissions and limitations under the
# License.
# 

import time, ntpath
from lib.utils import *
from lib.events import *

def paraverHeader(csvfile, timeMult=1, nthreads=1, duration=-1, delimiter=",", timeunit="ns"):
    
    csvfile.seek(0)

    nodeCPU = [1]
    nodeTask = [1]


    line = "#Paraver ("+time.strftime("%d/%m/%Y")+" at "+time.strftime("%H:%M")+"):"
    
    if (duration == -1) :
        ###Get trace duration###
        data = csv.reader(csvfile, skipinitialspace=True, delimiter=delimiter)
        count = 0
        first = 0
        last = 0
        for row in data:
             if (sanityCheck(row, verbose=False) == 1) :
                 if (count == 0) : 
                     count = 1
                     first = ffloat(row[0])*timeMult
                 else : 
                     last = row[0]
     
        last = ffloat(last)*timeMult
        duration = int(last-first)
        ########################
    else :
        duration = int(ffloat(duration)*timeMult)

    line += str(duration)+"_"+timeunit+":1("+str(nthreads)+"):1:1:("+str(nthreads)+":1)\n"
    return line


def createRow(csv_filename, directory, threadName, nthreads=1, useNumbers=False) :
    ###Get row file name###
    filename = os.path.splitext(csv_filename)[0]
    filename += ".row"
    filename = os.path.basename(filename)
    #######################

    ####Hierarchy dependant code####
    lvl_cpu = nthreads
    ncpus = nthreads
    ntasks = 1
    nnodes = 1
    ###############################
    rowFile = open(directory + "/" +filename, "w+")

    #LEVEL CPU
    rowFile.write("LEVEL CPU SIZE "+str(lvl_cpu)+"\n")
    for th in range(1, ncpus+1) :
        if (th < 10):
            rowFile.write("0"+str(th)+"."+str(threadName)+"\n")
        else :
            rowFile.write(str(th)+"."+str(threadName)+"\n")
    rowFile.write("\n")

    #LEVEL TASK
    rowFile.write("LEVEL TASK SIZE "+str(ntasks)+"\n")
    rowFile.write(str(threadName)+"\n")
    rowFile.write("\n")

    #LEVEL NODE
    rowFile.write("LEVEL NODE SIZE "+str(nnodes)+"\n")
    rowFile.write(str(threadName)+"\n")
    rowFile.write("\n")

    #LEVEL THREAD
    rowFile.write("LEVEL THREAD SIZE "+str(nthreads)+"\n")
    if (useNumbers == False) :
        for th in range(1, nthreads+1) :
            rowFile.write(str(threadName)+"\n")
    else :
        for th in range(1, nthreads+1) :
            rowFile.write(str(th)+"\n")

    printMsg("ROW generated successfully!", "OK")
    rowFile.close()
    return

def getEventsInPcfFormat(prvEvents):

    line = ""

    for event in prvEvents:
        line = line + "\nEVENT_TYPE\n"

        line = line + "9   " + str(event.id) + "     " + event.name  +"\n"

        if len(event.values)!=0:
            line = line + "VALUES\n"

            for label, value in event.values.items():
               line = line + str(value) + " "  + label + "\n"

            line= line + "\n"

        if event.derivedEvents != None:
            line = line + getEventsInPcfFormat(event.derivedEvents)

    return line


def createPcf(csv_filename, directory, prvEvents):

    ###Get row file name###
    filename = os.path.splitext(csv_filename)[0]
    filename += ".pcf"
    filename = os.path.basename(filename)
    #######################

    pcfFile = open(directory+"/"+filename, "w+")

    pcfFile.write("DEFAULT_OPTIONS\n\n"
            "LEVEL               THREAD\n"
            "UNITS               NANOSEC\n"
            "LOOK_BACK           100\n"
            "SPEED               1\n"
            "FLAG_ICONS          ENABLED\n"
            "NUM_OF_STATE_COLORS 1000\n"
            "YMAX_SCALE          37\n\n\n"
            "DEFAULT_SEMANTIC\n\n"
            "THREAD_FUNC          State As Is\n\n\n"
            "STATES\n"
            "0    Idle\n"
            "1    Running\n"
            "2    Not created\n"
            "3    Waiting a message\n"
            "4    Blocking Send\n"
            "5    Synchronization\n"
            "6    Test/Probe\n"
            "7    Scheduling and Fork/Join\n"
            "8    Wait/WaitAll\n"
            "9    Blocked\n"
            "10    Immediate Send\n"
            "11    Immediate Receive\n"
            "12    I/O\n"
            "13    Group Communication\n"
            "14    Tracing Disabled\n"
            "15    Others\n"
            "16    Send Receive\n"
            "17    Memory transfer\n"
            "18    Profiling\n"
            "19    On-line analysis\n"
            "20    Remote memory access\n"
            "21    Atomic memory operation\n"
            "22    Memory ordering operation\n"
            "23    Distributed locking\n"
            "24    Overhead\n"
            "25    One-sided op\n"
            "26    Startup latency\n"
            "27    Waiting links\n"
            "28    Data copy\n"
            "29    RTT\n"
            "30    Allocating memory\n"
            "31    Freeing memory\n\n\n"
            "STATES_COLOR\n"
            "0    {117,195,255}\n"
            "1    {0,0,255}\n"
            "2    {255,255,255}\n"
            "3    {255,0,0}\n"
            "4    {255,0,174}\n"
            "5    {179,0,0}\n"
            "6    {0,255,0}\n"
            "7    {255,255,0}\n"
            "8    {235,0,0}\n"
            "9    {0,162,0}\n"
            "10    {255,0,255}\n"
            "11    {100,100,177}\n"
            "12    {172,174,41}\n"
            "13    {255,144,26}\n"
            "14    {2,255,177}\n"
            "15    {192,224,0}\n"
            "16    {66,66,66}\n"
            "17    {255,0,96}\n"
            "18    {169,169,169}\n"
            "19    {169,0,0}\n"
            "20    {0,109,255}\n"
            "21    {200,61,68}\n"
            "22    {200,66,0}\n"
            "23    {0,41,0}\n"
            "24    {139,121,177}\n"
            "25    {116,116,116}\n"
            "26    {200,50,89}\n"
            "27    {255,171,98}\n"
            "28    {0,68,189}\n"
            "29    {52,43,0}\n"
            "30    {255,46,0}\n"
            "31    {100,216,32}\n\n\n")

    
    line = getEventsInPcfFormat(prvEvents)
            
    pcfFile.write(line)

    printMsg("PCF generated successfully!", "OK")
    pcfFile.close()
    return
