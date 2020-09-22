#!/usr/bin/python3
import csv, os, gzip, decimal, re

def printMsg(message, msgType="WARNING") :
    class bcolors:
        HEADER = '\033[95m'
        INFO = '\033[94m'
        OKGREEN = '\033[92m'
        WARNING = '\033[93m'
        ERROR = '\033[91m'
        ENDC = '\033[0m'
    
    if (msgType == "OK") : print(bcolors.OKGREEN +"OK: "+bcolors.ENDC+message)
    elif (msgType == "ERROR") : print(bcolors.ERROR +"Error: "+message+bcolors.ENDC)
    elif (msgType == "INFO") : print(bcolors.INFO +"INFO: "+bcolors.ENDC+message)
    #else : print(bcolors.WARNING +"Warning: "+bcolors.ENDC+message)
    else : print(bcolors.WARNING +"Warning: "+bcolors.ENDC+message)

def sanityCheck(row, verbose=True) :
    # skip if line is empty
    if len(row) == 0:
      if (verbose) : printMsg("Found a blank line.")
      return -1
    
    # skip the following lines until a digit is found
    #   --> line that are not starting with a number should be skipped
    if re.match("[^0-9-]",row[0]):
        if (verbose) : printMsg("Skipping line "+str(row))
        return -1

    return 1

def openCSVFile(filename, info=True, mode="r"):
    if (info == True):
        printMsg("Reading file "+str(filename), "INFO")
    #get file extension
    f_ext = os.path.splitext(os.path.basename(filename))

    if f_ext[1] == ".gz":
        return io.TextIOWrapper(gzip.open(filename, mode))
    else:
        return open(filename, mode)

def ffloat(a):
    return decimal.Decimal(a)

def openPRVFile(filename, directory, verbose=True, compress=True, compress_lvl=6):
    basename = directory + "/" + os.path.splitext(os.path.basename(str(filename)))[0]
    #basename = os.path.splitext(os.path.basename(str(basename)))[0]
    if (compress) : 
        outputname = basename+".prv.gz"
        if (verbose): printMsg("Generating trace "+outputname, "INFO")
        return gzip.open(outputname, "w+", compresslevel=compress_lvl)
    else :
        outputname = basename+".prv"
        if (verbose): printMsg("Generating trace "+outputname, "INFO")
        return open(outputname, "w+")

def writePRVFile(prvFile, line):
    if (isinstance(prvFile, gzip.GzipFile)):
        line = bytes(line, "ascii")
        prvFile.write(line)
    else :
        prvFile.write(line)

def writeMultPRV(prvFile, multIDs, multValues, time):
    events = "2:1:1:1:1:" + str(time)
    for multID, multValue in zip(multIDs, multValues):
        events += ":" + str(multID) + ":" + str(multValue)
    writePRVFile(prvFile, events)

