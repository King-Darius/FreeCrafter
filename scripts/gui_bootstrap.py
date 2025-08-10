import subprocess
import sys
import threading

import PySimpleGUI as sg

import bootstrap


def run_bootstrap(window):
    """Run the bootstrap script and stream its output to the GUI."""
    cmd = [sys.executable, bootstrap.__file__]
    process = subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        bufsize=1,
    )

    def reader(stream, event):
        for line in iter(stream.readline, ""):
            window.write_event_value(event, line)
        stream.close()

    threading.Thread(target=reader, args=(process.stdout, "-STDOUT-"), daemon=True).start()
    threading.Thread(target=reader, args=(process.stderr, "-STDERR-"), daemon=True).start()

    process.wait()
    window.write_event_value("-DONE-", process.returncode)


def main():
    layout = [
        [sg.Multiline(
            size=(80, 20), key="-OUTPUT-", autoscroll=True, disabled=True
        )],
        [sg.Button("Install"), sg.Button("Exit")],
    ]

    window = sg.Window("FreeSketch Installer", layout)

    while True:
        event, values = window.read()
        if event in (sg.WIN_CLOSED, "Exit"):
            break
        if event == "Install":
            window["-OUTPUT-"].update("")
            threading.Thread(target=run_bootstrap, args=(window,), daemon=True).start()
        elif event == "-STDOUT-":
            window["-OUTPUT-"].print(values[event], end="")
        elif event == "-STDERR-":
            window["-OUTPUT-"].print(values[event], text_color="red", end="")
        elif event == "-DONE-":
            rc = values[event]
            if rc == 0:
                window["-OUTPUT-"].print("\nInstall complete.")
            else:
                window["-OUTPUT-"].print(f"\nInstall failed with code {rc}", text_color="red")

    window.close()


if __name__ == "__main__":
    main()
