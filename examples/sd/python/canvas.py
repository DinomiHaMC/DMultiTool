import tkinter as tk
import device

root = tk.Tk()
root.title("Canvas")
canvas = tk.Canvas(root)
canvas.pack()
x = 0

def tick():
    global x
    canvas.delete("all")
    canvas.create_rectangle(x, 20, x + 24, 44, fill="green")
    x = (x + 4) % (device.width() - 24)
    root.after(100, tick)

root.after(100, tick)
root.mainloop()
