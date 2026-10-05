import tkinter as tk
import device

root = tk.Tk()
root.title("IR + microSD")
status = tk.Label(root, text="Own hardware demo")
status.pack()

def send_ir():
    ok = device.ir_nec(0x0000, 0x10, 0)
    status.config(text="IR queued: " + str(ok))

def save_file():
    ok = device.write_text("demo.txt", "Hello from Python\n")
    status.config(text="SD saved: " + str(ok))

def read_file():
    status.config(text=device.read_text("demo.txt"))

tk.Button(root, text="Send own NEC 0000/10", command=send_ir).pack()
tk.Button(root, text="Write /python/demo.txt", command=save_file).pack()
tk.Button(root, text="Read /python/demo.txt", command=read_file).pack()
root.mainloop()
