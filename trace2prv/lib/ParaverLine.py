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

from enum import Enum

GET_LINE_SORTED=False

class ParaverTypeRecord(Enum):
    STATE         = 1
    EVENT         = 2
    COMMUNICATION = 3

class ParaverLine:

    def __init__(self, typeRecord, cpuId, applId, taskId, threadId, time=0):
        self.typeRecord  = typeRecord
        self.cpuId       = cpuId
        # self.applId      = taskId
        # self.taskId      = threadId
        self.applId      = applId
        self.taskId      = taskId        
        self.threadId    = threadId
        self.time        = time
        self.events      = []

    def addEvent(self, event_type, event_value):
        self.events.append([event_type, event_value])

    # If event is removed return value if not return None
    def removeEvent(self, paraver_event):
        for event in self.events:
            if event[0] == paraver_event:
                value = event[1]
                self.events.remove(event)
                return value
        return None

    def sortEvents(self):
        self.events = sorted(self.events, key = lambda x: (int(x[0].id), int(x[1])))

    def getLine(self):

        if self.GET_LINE_SORTED:
            self.sortEvents()

        line = str(self.typeRecord) + ":"  + str(self.cpuId) + ":"  + str(self.applId) + ":"  + str(self.taskId) + ":"  + str(self.threadId) + ":"  + str(self.time)
        for event in self.events:
            line = line + ":" + str(event[0].id) + ":" + str(event[1])

        line = line + "\n"
        return line

