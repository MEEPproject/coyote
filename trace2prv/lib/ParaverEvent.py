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

    def setMul(self, mul):

        if not isinstance(mul, int):
            raise Exception('Mul need to be an integer')
    
    def addDerivedEvent(self, prvEvent):

        if not isinstance(prvEvent, PrvEvent):
            raise Exception('DerivedEvent need to be PrvEvents')

        self.derivedEvents.append(prvEvent)
