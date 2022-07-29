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

import json, sys, os, re
from lib.ParaverEvent import *

def getFromJson(pathToJson, fieldName):
    with open(pathToJson) as f:
        data = json.load(f)
    return data[fieldName]

def getInstructionsFromJson(pathToJson):
    return getFromJson(pathToJson, "Instruction")

def assignValuesFromJson(pathToJson, fieldName, paraverEvent):
    labels = getFromJson(pathToJson, fieldName)
    number_values = len(paraverEvent.values)+1
    for label in labels:
        paraverEvent.addValue(int(number_values), label)
        number_values = number_values + 1

def getRegistersValuesFromJson(pathToJson, registers):

    raw_registers = getFromJson(pathToJson, "Registers")
    registers.addValue(1,"zero") 
    registers.addValue(2,"ra")
    registers.addValue(3,"sp")
    registers.addValue(4,"gp")
    registers.addValue(5,"tp")
    registers.addValue(6,"t1")
    registers.addValue(7,"t2")
    registers.addValue(8,"fp")

    i=9
    for reg in raw_registers:
        type_index=0;

        if(not reg[1].isdigit()):
            type_index=1

        type_reg=reg[:type_index+1]
        numbers=reg[type_index+1:].split('-')
        if(len(numbers)==1):
            registers.addValue(i, reg)
            i = i+1
        else:
            for n in range(int(numbers[0]), int(numbers[1])+1):
                registers.addValue(i, type_reg+str(n))
                i = i+1


def getInstruction(disasm):
    return  re.search("^(\w*)(\.|\t|\n|\s|$)",disasm).group(1)

def isVectoriaInstruction(disasm):
    return  re.search("^v",disasm) != None

def getInstructionPostfix(disasm):
    operation = disasm.split(" ")[0]
    postfix = re.search("\..*",operation)

    if postfix == None:
        return None
    else:
        return postfix.group(0)[1:]

