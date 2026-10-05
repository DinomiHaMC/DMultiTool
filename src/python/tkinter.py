# Compact TFT adapter. This is not desktop Tcl/Tk.
import device

class Event:
    def __init__(self, key):
        self.keysym = key

class Tk:
    def __init__(self):
        self.running = True
        self.handlers = {}
        self.actions = {}
        self.timer = None
        self.deadline = 0
    def title(self, text):
        device.title(text)
    def bind(self, key, callback):
        self.handlers[key] = callback
    def after(self, ms, callback):
        self.deadline = device.millis() + ms
        self.timer = callback
    def update(self):
        key = device.button()
        if key != "":
            names = {"UP":"<Up>","DOWN":"<Down>","LEFT":"<Left>","RIGHT":"<Right>","OK":"<Return>"}
            event = Event(key)
            if "<Key>" in self.handlers:
                callback = self.handlers["<Key>"]
                callback(event)
            if names[key] in self.handlers:
                callback = self.handlers[names[key]]
                callback(event)
            if key == "OK":
                selected = device.selected()
                if str(selected) in self.actions:
                    callback = self.actions[str(selected)]
                    callback()
        if self.timer != None:
            if device.millis() >= self.deadline:
                callback = self.timer
                self.timer = None
                callback()
    def mainloop(self):
        while self.running and device.alive():
            self.update()
            device.sleep_ms(20)
    def destroy(self):
        self.running = False
        device.destroy()

class Label:
    def __init__(self, master, text=""):
        self.id = device.label(text)
    def pack(self):
        pass
    def config(self, text):
        device.set_text(self.id, text)
    def configure(self, text):
        self.config(text)

class Button:
    def __init__(self, master, text="", command=None):
        self.id = device.button_widget(text)
        if command != None:
            master.actions[str(self.id)] = command
    def pack(self):
        pass
    def config(self, text):
        device.set_text(self.id, text)

class Canvas:
    def __init__(self, master, width=0, height=0):
        self.width = device.width()
        self.height = device.height()
    def pack(self):
        pass
    def create_rectangle(self, x1, y1, x2, y2, fill="white"):
        colors = {"black":0,"white":65535,"red":63488,"green":2016,"blue":31,"yellow":65504,"cyan":2047,"magenta":63519}
        if fill in colors:
            color = colors[fill]
        else:
            rgb = int(fill[1:], 16)
            color = ((rgb >> 19) << 11) | (((rgb >> 10) & 63) << 5) | ((rgb >> 3) & 31)
        return device.rectangle(x1, y1, x2, y2, color)
    def delete(self, tag):
        device.clear_canvas()
