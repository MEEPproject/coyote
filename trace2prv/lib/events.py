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

import json, sys, os
from lib.utils import *  
from lib.ParaverEvent import *
from lib.genericCSVtoPRV import *
from lib.ParserFunctions import *


class Event(PrvEvent):

    def __init__(self, name, event_id, mul=1, parser_function="eventIntegerParser", args=[]):

        PrvEvent.__init__(name, event_id)
        self.setMul(mul)
        self.parser_function = parser_function
        self.parser_functions_args = args

    def getValue(self, textToParse):
        return self.parser_function(textToParse, self.parser_functions_args)


def getStandardID(idType, localEventID):
    return "47" + '{0:02d}'.format(int(idType)) + '{0:04d}'.format(int(localEventID))


def getMulFromJson(event_json):

    multiplication_key = "multiplicator"
    multiplication_value = 1
    if multiplication_key in event_json:
        multiplication_value = int(event_json[multiplication_key]);

    return multiplication_value

def getDerivedEvents(json_data_event):

    _derived_events_key = "derivedEvents"

    return getFromJson(json_data_event, _derived_events_key)

def getParseFunction(json_data_event, default_function="eventIntegerParser"):

    _function_key = "function"

    _function = getFromJson(json_data_event, _function_key)
    if _function is None:
        _function = default_function

    _function = globals()[_function]
    return _function



def getFromJson(json_data_event, key):

    _data = None

    if key in json_data_event:
        _data = json_data_event[key]

    return _data


def getEvents(jsonDataEvents):

    PrvEvents = []

    for event in jsonDataEvents:
    
        eventName = event["eventName"]
        local_id = event["eventID"]
        tmp_event = PrvEvent(eventName, local_id)
        multiplicator = getMulFromJson(event)
        getParseFunction(event)
        tmp_event.setMul(multiplicator)
        derived_events = getDerivedEvents(event)

        if derived_events != None:
           derived_paraver_events = getEvents(derived_events)
           for derived_paraver_event in derived_paraver_events:
               tmp_event.addDerivedEvent(derived_paraver_event)

    
        PrvEvents.append(tmp_event);

    
    return PrvEvents


def getEventsFromJson(dataJson):

    events_key = "events"
    eventsJson = dataJson[events_key]
    prvEvents = getEvents(eventsJson)

    return prvEvents

  
def getLocalTypeID(jsonData, csvTypeFile):

    prefix_key = "machineId"
    local_prefix_value = jsonData[prefix_key]

    return local_prefix_value



#Get Json string for a individial type of csv
def getJsonForIndividialTypeFromFile(path, csvTypeFile):

    with open(path) as f:
        data = json.load(f)

    if csvTypeFile not in data:
        raise Exception ("Error: Csv type not found")

    json_data = data[csvTypeFile]
    f.close()

    return json_data

def changeIdtoStd(events, stdId):
    for event in events:
        local_id = event.id
        event.id = getStandardID( stdId, local_id)
        derived_events = event.derivedEvents
        if derived_events != None:
            changeIdtoStd(derived_events, stdId)

    return events

def eventsAreInCsvColumns(prvEvents, nameColumns):

    for name_column in nameColumns:
        find = False
        for prvEvent in prvEvents:
            if prvEvent.name == name_column:
                find = True
                break

    return True



def printEvents(paraver_events, deep):

    indent='-'*(2*deep)
    for paraver_event in paraver_events:
        print(indent+paraver_event.name)
        if paraver_event.derivedEvents != None:
            printEvents(paraver_event.derivedEvents, deep+1)


def getStdEventsFromJsonFile(eventNames, csvTypeFile):

    eventsPyPath = os.path.dirname(os.path.realpath(__file__))
    eventsPyPath = eventsPyPath+"/events.json"
    json_data = getJsonForIndividialTypeFromFile(eventsPyPath, csvTypeFile)
    events = getEventsFromJson(json_data)
    stdId = getLocalTypeID(json_data, csvTypeFile)
    stdEvents = changeIdtoStd(events, stdId)

    eventsAreInCsvColumns(events, eventNames)

    return stdEvents
