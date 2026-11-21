#!/usr/bin/env python3
"""Window (tkinter) for stlinkv2_dfu.py: program a firmware image into an ST-LINK/V2 through its
own bootloader, or make a signed image for an SWD programmer.

    python stlinkv2_dfu_gui.py                  opens the window
    python stlinkv2_dfu_gui.py firmware.hex     opens the window with that file selected
    python stlinkv2_dfu_gui.py --cli <args>     command line, the same arguments as stlinkv2_dfu.py

The protocol, the crypto and the image handling live in stlinkv2_dfu.py; this file only adds
the window. With PyInstaller it becomes a single Windows executable (see README.md).
"""
import os
import queue
import sys
import threading
import time
import traceback

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import stlinkv2_dfu as core  # noqa: E402

APP_NAME = "ST-LINK/V2 DFU Flasher"


def app_dir():
    """Folder of the executable (PyInstaller build) or of this script."""
    if getattr(sys, "frozen", False):
        return os.path.dirname(sys.executable)
    return HERE


def friendly_error(e):
    """Turn USB / driver errors into a message the user can act on.

    e:       the exception
    returns: text for the log and the error dialog
    """
    text = str(e)
    name = type(e).__name__
    if name == "NoBackendError":
        return "libusb is missing - pip install libusb-package (or reinstall the executable)."
    if "Access denied" in text or "Operation not supported" in text or "NotImplemented" in name \
            or "not supported" in text.lower():
        return ("The operating system does not let this tool open the probe. On Windows install "
                "ST's ST-LINK USB driver (STSW-LINK009); on Linux add the udev rules of stlink."
                "\n\n(" + text + ")")
    if "timed out" in text.lower() or "timeout" in text.lower():
        return ("The probe stopped answering. Unplug it, plug it back in and try again; "
                "it stays in DFU mode until programming completes.\n\n(" + text + ")")
    return text


def run_cli(argv):
    """Command-line mode; on Windows the output goes to the console the tool was started from
    (a windowed PyInstaller build has none of its own). Returns the exit code."""
    if os.name == "nt":
        try:
            import ctypes
            if ctypes.windll.kernel32.AttachConsole(-1):
                sys.stdout = open("CONOUT$", "w")
                sys.stderr = sys.stdout
        except Exception:
            pass
    try:
        return core.main(argv)
    except core.FlashError as e:
        print("ERROR:", e)
        return 1


def run_gui(initial_file=None):
    """Build the window and run it until it is closed.

    initial_file: firmware file to preselect, or None
    """
    import tkinter as tk
    from tkinter import ttk, filedialog, messagebox, scrolledtext, simpledialog

    root = tk.Tk()
    root.title(APP_NAME)
    root.minsize(640, 460)

    q = queue.Queue()                       # worker thread -> window: ("log" | "progress" | "done" | "error", value)
    state = {"busy": False, "cancel": False, "backend": None, "mem": None}

    # ---- layout
    frm = ttk.Frame(root, padding=10)
    frm.pack(fill="both", expand=True)
    frm.columnconfigure(1, weight=1)
    frm.rowconfigure(5, weight=1)

    ttk.Label(frm, text="Firmware file:").grid(row=0, column=0, sticky="w")
    file_var = tk.StringVar(value=initial_file or "")
    file_entry = ttk.Entry(frm, textvariable=file_var)
    file_entry.grid(row=0, column=1, sticky="ew", padx=6)
    browse_btn = ttk.Button(frm, text="Browse...")
    browse_btn.grid(row=0, column=2)

    info_var = tk.StringVar(value="")
    ttk.Label(frm, textvariable=info_var, foreground="#555").grid(row=1, column=1, columnspan=2,
                                                                   sticky="w", padx=6)

    ttk.Label(frm, text="Probe:").grid(row=2, column=0, sticky="w", pady=(8, 0))
    probe_var = tk.StringVar(value="looking ...")
    ttk.Label(frm, textvariable=probe_var).grid(row=2, column=1, columnspan=2, sticky="w",
                                                padx=6, pady=(8, 0))

    btns = ttk.Frame(frm)
    btns.grid(row=3, column=0, columnspan=3, sticky="ew", pady=10)
    program_btn = ttk.Button(btns, text="Program")
    program_btn.pack(side="left")
    cancel_btn = ttk.Button(btns, text="Cancel", state="disabled")
    cancel_btn.pack(side="left", padx=6)
    swd_btn = ttk.Button(btns, text="Make SWD image...")
    swd_btn.pack(side="right")
    status_var = tk.StringVar(value="Ready")
    ttk.Label(btns, textvariable=status_var).pack(side="left", padx=10)

    progress = ttk.Progressbar(frm, mode="determinate", maximum=100)
    progress.grid(row=4, column=0, columnspan=3, sticky="ew")

    log_box = scrolledtext.ScrolledText(frm, height=14, state="disabled", wrap="word",
                                        font=("Consolas", 9))
    log_box.grid(row=5, column=0, columnspan=3, sticky="nsew", pady=(8, 0))

    def log(msg):
        """Append a time-stamped line to the log box."""
        log_box.configure(state="normal")
        log_box.insert("end", time.strftime("%H:%M:%S  ") + msg + "\n")
        log_box.see("end")
        log_box.configure(state="disabled")

    # ---- image
    def load_selected():
        """Load and check the selected file; show what it is under the file name."""
        path = file_var.get().strip()
        state["mem"] = None
        if not path:
            info_var.set("Choose the .hex or .bin file to program.")
            return
        try:
            mem = core.load_image(path)
            core.check_image(mem)
        except (OSError, ValueError, core.FlashError) as e:
            info_var.set(f"Not usable: {e}")
            return
        used = len(bytes(mem[0x3C00:]).rstrip(b"\xff"))
        _, rv = core.struct.unpack_from("<II", mem, core.APP_ADDR - core.FLASH_BASE)
        layout = "ST layout" if core.has_st_layout(mem) else "application only"
        info_var.set(f"OK - {layout}, {used // 1024} KB to write, needs a probe with "
                     f"{core.needed_kb(mem)}+ KB flash, reset vector 0x{rv:08X}")
        state["mem"] = mem

    def browse():
        """File dialog for the firmware file."""
        p = filedialog.askopenfilename(
            title="Firmware file",
            filetypes=[("Firmware", "*.hex *.bin"), ("Intel HEX", "*.hex"), ("Binary", "*.bin"),
                       ("All files", "*.*")],
            initialdir=os.path.dirname(file_var.get()) or app_dir())
        if p:
            file_var.set(p)
            load_selected()

    browse_btn.configure(command=browse)
    file_entry.bind("<FocusOut>", lambda e: load_selected())
    file_entry.bind("<Return>", lambda e: load_selected())

    # ---- probe polling (USB IDs only, never talks to the probe, so other tools are not disturbed)
    def poll_probe():
        if not state["busy"]:
            try:
                if state["backend"] is None:
                    state["backend"] = core.usb_backend()
                probe_var.set("ST-LINK/V2 connected (0483:3748)" if core.probe_present(state["backend"])
                              else "No probe found - plug the ST-LINK/V2 into USB")
            except Exception as e:
                probe_var.set("USB not available: " + friendly_error(e))
        root.after(1000, poll_probe)

    # ---- programming (in a worker thread; results come back through q)
    def worker(mem):
        def wlog(m):
            q.put(("log", m))

        def wprog(i, n):
            q.put(("progress", i * 100.0 / n))
        try:
            res = core.install(mem, log=wlog, progress=wprog, cancel=lambda: state["cancel"],
                               be=state["backend"])
            q.put(("done", res))
        except core.Cancelled as e:
            q.put(("error", str(e)))
        except core.FlashError as e:
            q.put(("error", friendly_error(e)))
        except Exception as e:                                   # USB / driver problems
            q.put(("log", traceback.format_exc().strip().splitlines()[-1]))
            q.put(("error", friendly_error(e)))

    def set_busy(b):
        """Enable / disable the controls while programming runs."""
        state["busy"] = b
        for w in (program_btn, browse_btn, swd_btn, file_entry):
            w.configure(state="disabled" if b else "normal")
        cancel_btn.configure(state="normal" if b else "disabled")

    def start():
        load_selected()
        if state["mem"] is None:
            messagebox.showerror(APP_NAME, info_var.get() or "Choose a firmware file first.")
            return
        if not messagebox.askyesno(
                APP_NAME,
                "Program this file into the connected ST-LINK/V2?\n\n" + file_var.get() +
                "\n\nDo not unplug the probe while it is being programmed "
                "(unless the tool asks you to replug it)."):
            return
        try:
            if state["backend"] is None:
                state["backend"] = core.usb_backend()
        except Exception as e:
            messagebox.showerror(APP_NAME, friendly_error(e))
            return
        state["cancel"] = False
        progress["value"] = 0
        set_busy(True)
        status_var.set("Programming ...")
        log(f"Programming {os.path.basename(file_var.get())}")
        threading.Thread(target=worker, args=(state["mem"],), daemon=True).start()

    def cancel():
        state["cancel"] = True
        status_var.set("Cancelling ...")

    def make_swd():
        """Ask for the probe UID and save the selected full image signed for that probe."""
        load_selected()
        if state["mem"] is None:
            messagebox.showerror(APP_NAME, info_var.get() or "Choose a firmware file first.")
            return
        text = simpledialog.askstring(
            APP_NAME, "Probe UID - 24 hex digits, the 12 bytes at 0x1FFFF7E8 of the probe's STM32\n"
                      "(read them with any SWD programmer):", parent=root)
        if not text:
            return
        try:
            uid = core.parse_uid(text)
            img = core.make_swd_image(state["mem"], uid)
        except core.FlashError as e:
            messagebox.showerror(APP_NAME, str(e))
            return
        path = filedialog.asksaveasfilename(
            title="Save SWD image", defaultextension=".hex", initialfile=f"stlinkv2_{uid.hex()}.hex",
            filetypes=[("Intel HEX", "*.hex"), ("Binary from 0x08000000", "*.bin")])
        if not path:
            return
        if path.lower().endswith(".bin"):
            with open(path, "wb") as f:
                f.write(bytes(img))
        else:
            core.write_hex(path, img)
        log(f"Saved SWD image for UID {uid.hex()}: {path}")
        messagebox.showinfo(APP_NAME, "Saved:\n" + path + "\n\nWrite it at 0x08000000 with an SWD "
                            "programmer (mass erase first).")

    swd_btn.configure(command=make_swd)
    program_btn.configure(command=start)
    cancel_btn.configure(command=cancel)

    def pump():
        """Take the worker's messages from q and show them (runs every 50 ms)."""
        try:
            while True:
                kind, val = q.get_nowait()
                if kind == "log":
                    log(val)
                    if "plug it back in" in val:
                        status_var.set("Unplug the probe and plug it back in")
                elif kind == "progress":
                    progress["value"] = val
                    status_var.set(f"Programming ... {int(val)}%")
                elif kind == "done":
                    set_busy(False)
                    progress["value"] = 100
                    status_var.set("Done")
                    messagebox.showinfo(APP_NAME, "Programming finished.")
                elif kind == "error":
                    set_busy(False)
                    status_var.set("Failed")
                    log("ERROR: " + val)
                    messagebox.showerror(APP_NAME, val)
        except queue.Empty:
            pass
        root.after(50, pump)

    def on_close():
        if state["busy"] and not messagebox.askyesno(
                APP_NAME, "Programming is running. Quit anyway?\n"
                          "(The probe stays in DFU mode; run the tool again to finish.)"):
            return
        root.destroy()

    root.protocol("WM_DELETE_WINDOW", on_close)

    if not core.selftest():
        messagebox.showerror(APP_NAME, "Internal crypto self-test failed.")
    load_selected()
    log(f"{APP_NAME} - programs ST-LINK/V2 probes through their own bootloader.")
    log("A probe that runs its application has to be replugged once to start the bootloader.")
    poll_probe()
    pump()
    root.mainloop()


if __name__ == "__main__":
    args = sys.argv[1:]
    if args and args[0] == "--cli":
        sys.exit(run_cli(args[1:]))
    if any(a.startswith("-") for a in args):          # e.g. --selftest, --list, x.hex --make-swd-image UID
        sys.exit(run_cli(args))
    run_gui(args[0] if args and os.path.isfile(args[0]) else None)
