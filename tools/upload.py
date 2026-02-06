#!/usr/bin/env python3
import json
import os
import re
import socket
import http.client
import subprocess
import sys
import time

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
CFG_PATH = os.path.join(ROOT, "tools", "upload.local.json")
STATE_PATH = os.path.join(ROOT, "tools", ".upload.state.json")
SETTINGS_CPP = os.path.join(ROOT, "src", "Settings.cpp")


def die(msg):
    print(msg, file=sys.stderr)
    sys.exit(1)


def load_json(path):
    if not os.path.exists(path):
        return {}
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)


def save_json(path, data):
    with open(path, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=2)


def load_config():
    if not os.path.exists(CFG_PATH):
        return {}
    return load_json(CFG_PATH)


def parse_clock_names_from_settings():
    if not os.path.exists(SETTINGS_CPP):
        return []
    text = open(SETTINGS_CPP, "r", encoding="utf-8").read()
    return re.findall(r"\{\s*\"([^\"]+)\"", text)


def merge_clocks(settings_names, local_clocks):
    by_name = {c.get("name"): c for c in local_clocks if c.get("name")}
    merged = []
    for name in settings_names:
        if name in by_name:
            merged.append(by_name[name])
        else:
            merged.append({"name": name})
    return merged if merged else local_clocks


def choose(options, prompt, default=None):
    print(prompt)
    for i, opt in enumerate(options, 1):
        print(f"{i}. {opt}")
    print("0. Exit")
    while True:
        raw = input("Select number: ").strip()
        if raw == "0":
            sys.exit(0)
        if raw == "" and default in options:
            return default
        if not raw.isdigit():
            print("Enter a number.")
            continue
        idx = int(raw)
        if 1 <= idx <= len(options):
            return options[idx - 1]
        print("Out of range.")


def list_usb_ports():
    try:
        out = subprocess.check_output(
            ["/bin/sh", "-c", "ls /dev/cu.* 2>/dev/null"],
            text=True,
        )
        return [p.strip() for p in out.splitlines() if p.strip()]
    except subprocess.CalledProcessError:
        return []


def run_pio(args):
    cmd = ["pio"] + args
    print("Running:", " ".join(cmd))
    subprocess.check_call(cmd, cwd=ROOT)


def mdns_name_from_label(label):
    if not label:
        return None
    if label.endswith(".local"):
        return label
    safe = label.replace(" ", "_")
    safe = "".join(ch for ch in safe if ch.isalnum() or ch in ("_", "-"))
    return f"{safe}.local" if safe else None


def wait_for_http(host, timeout_s=90):
    deadline = time.time() + timeout_s
    while time.time() < deadline:
        try:
            # Try TCP connect first
            with socket.create_connection((host, 80), timeout=2):
                pass
            # Then try a simple HTTP GET to be sure web server is up
            conn = http.client.HTTPConnection(host, 80, timeout=2)
            conn.request("GET", "/getDeviceName")
            resp = conn.getresponse()
            resp.read()
            conn.close()
            if resp.status == 200:
                return True
        except OSError:
            time.sleep(2)
    return False


def main():
    cfg = load_config()
    local_clocks = cfg.get("clocks", [])

    settings_names = parse_clock_names_from_settings()
    clocks = merge_clocks(settings_names, local_clocks)
    if not clocks:
        die("No clocks configured in tools/upload.local.json or Settings.cpp")

    state = load_json(STATE_PATH)
    last_clock = state.get("last_clock")

    names = [c.get("name", "(unnamed)") for c in clocks]
    if last_clock in names:
        print(f"Last used: {last_clock} (press Enter to reuse)")
    choice = choose(names, "Which clock?", default=last_clock)
    clock = clocks[names.index(choice)]

    name = clock.get("name", "").strip()
    derived_ota = mdns_name_from_label(name)
    ota_target = clock.get("ota_host") or derived_ota

    methods = []
    if ota_target:
        methods.append("OTA")
    methods.append("USB")
    method = choose(methods, "Upload method?")

    actions = ["Firmware only", "Firmware + Filesystem", "Filesystem only"]
    action = choose(actions, "What to upload?")

    if method == "OTA":
        if action in ("Firmware only", "Firmware + Filesystem"):
            run_pio(["run", "-e", "nodemcuv2_ota", "-t", "upload", "--upload-port", ota_target])
            if action == "Firmware + Filesystem":
                print("Waiting for device to reboot before filesystem upload...")
                if not wait_for_http(ota_target, timeout_s=90):
                    die("Device not reachable yet. Try 'Filesystem only' again in a moment.")
        if action in ("Filesystem only", "Firmware + Filesystem"):
            # One retry helps if the device is still rebooting.
            try:
                time.sleep(5)
                run_pio(["run", "-e", "nodemcuv2_ota", "-t", "uploadfs", "--upload-port", ota_target])
            except subprocess.CalledProcessError:
                print("Filesystem upload failed, retrying in 5 seconds...")
                time.sleep(5)
                run_pio(["run", "-e", "nodemcuv2_ota", "-t", "uploadfs", "--upload-port", ota_target])
    else:
        ports = list_usb_ports()
        if not ports:
            die("No /dev/cu.* ports found. Plug in the board and try again.")
        upload_port = choose(ports, "Select USB port")
        if action in ("Firmware only", "Firmware + Filesystem"):
            run_pio(["run", "-e", "nodemcuv2", "-t", "upload", "--upload-port", upload_port])
        if action in ("Filesystem only", "Firmware + Filesystem"):
            run_pio(["run", "-e", "nodemcuv2", "-t", "uploadfs", "--upload-port", upload_port])

    save_json(STATE_PATH, {"last_clock": choice})


if __name__ == "__main__":
    try:
        main()
    except subprocess.CalledProcessError as e:
        die(f"Upload failed with exit code {e.returncode}")
