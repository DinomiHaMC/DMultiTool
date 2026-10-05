# PikaPython, embedded in Hacker Pro 2000

Upstream: https://github.com/pikasTech/PikaPython
Tag: v1.12.5; commit e4e458dee6b8bf6d6372749b39f861068e49520e.
License: MIT, retained in LICENSE and source headers.

Copied core `src/*.c,*.h`, native `package/PikaStdLib/*.c,*.h`, and generated binding headers / `__pikaBinding.c`. The Arduino sketch recursively compiles this directory. Generated `pikaScript.c` is replaced by `src/python/PythonEngine.c`; no generated main bytecode/archive is required. `bindings/` contains the inputs used with upstream `tools/pikaCompiler/rust-msc` (run that generator in the bindings directory and copy its `pikascript-api/__pikaBinding.c` and headers here).

Local changes:
- `pika_config_valid.h` enables the adjacent `pika_config.h` without changing global Arduino C flags.
- Size optimization, UTF-8 strings, no interpreter events/threads/caches/mark-sweep GC, disabled Debug/Task modules, instruction hook every 32 instructions, 1024-byte VM stack.
- `_find_super_class_name` in `PikaVM.c` uses `int32_t` for offset, matching the function signature on ESP32 toolchains.
- VM argument-list overflow and `dataStack.c` stack overflow call the platform error hook instead of an uninterruptible infinite loop.
- `PythonEngine.c` overrides allocation, errors, panic, instruction/yield hooks and console; allocation ownership, 64KB total budget, call-depth guard and C setjmp recovery permit cancellation/OOM cleanup. Every 1024 allocations yields to the scheduler, including source parsing.
- Built-in file opening and blocking console input are unavailable. SD is exposed explicitly through the queued `device` API; all SPI/IR work remains in the main firmware task.

The tiny TFT `tkinter` module is project-owned, not upstream desktop Tcl/Tk. Its Python source lives in `src/python/tkinter.py`; `TkinterSource.h` embeds identical lines as JSON-escaped adjacent C string literals. Keep them synchronized when editing.

Host validation: `tests/run-python.sh` links the real vendored interpreter to a simulated device backend and tests callbacks, cancellation, OOM, errors, depth limit, restart, and the shipped SD examples. Hardware/FreeRTOS integration still requires ESP32 bench testing.
