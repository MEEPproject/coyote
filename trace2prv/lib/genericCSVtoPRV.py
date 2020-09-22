import csv
from lib.utils import *
from lib.paraver import *


def readGenericCSVHeader(csvfile, delimiter):
    csvfile.seek(0)
    csvData = csv.reader(csvfile, skipinitialspace=True, delimiter=delimiter)
    eventNames = csvData.__next__()
    csvfile.seek(0)
    return eventNames


def genericGeneratePRV(csvfile, prvfile, sensorIDs, delimiter, verbose=True):
    
    #Generate Paraver header
    header = paraverHeader(csvfile, nthreads=1, delimiter=",")
    writePRVFile(prvfile, header)

    #Write mult events in time 0
    csvfile.seek(0)
    data = csv.reader(csvfile, skipinitialspace=True, delimiter=delimiter)
    tag="2:1:1:1:1:"
    first = 0
    firsttime = 0
    for row in data:
        #check for blanks or non-data rows
        if(sanityCheck(row, verbose) == -1): continue

        if(first == 0):
            first = 1
            firsttime = int(row[0])


        timestamp = int(row[0])-firsttime
        i = 0
        line = tag + str(timestamp)
        for col in row[1:]:
            i = i + 1
            if(i==2):
                continue
            val = int(col,0)
            line = line + ":" + sensorIDs[i] + ":" + str(val)
        
        writePRVFile(prvfile, line)

    printMsg("PRV generated successfully!", "OK")
