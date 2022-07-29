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

class PrvEvent:

    def __init__(self, name, id):
        self.name   = name
        self.id     = id
        self.mul    = 1
        self.values = dict()
        self.derivedEvents = []

    def __str__(self):
        return "PrvEvent(name={}, id={})".format(self.name, self.id)

    def addValue(self, value, label):

        if not isinstance(value, int):
            raise Exception('Value need to be an integer')

        if not isinstance(label, str):
            raise Exception('Label need to be a string')

        self.values[label] = value;

    def containsKey(self, key):
        return key in self.values

    def numValues(self):
        return len(list(self.values.values()))

    def getValue(self, key):
        return self.values.get(key)

    def setMul(self, mul):

        if not isinstance(mul, int):
            raise Exception('Mul need to be an integer')
    
    def addDerivedEvent(self, prvEvent):

        if not isinstance(prvEvent, PrvEvent):
            raise Exception('DerivedEvent need to be PrvEvents')

        self.derivedEvents.append(prvEvent)
