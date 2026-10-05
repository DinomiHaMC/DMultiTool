import tkinter as tk
import device

root = tk.Tk()
root.title("Buttons")
label = tk.Label(root, text="Press one of the 5 keys")
label.pack()

def on_key(event):
    label.config(text="Event: " + event.keysym)

def monitor():
    print("Held: " + device.pressed())
    root.after(500, monitor)

root.bind("<Key>", on_key)
root.after(500, monitor)
root.mainloop()
