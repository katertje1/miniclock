#!/usr/bin/env python3
import subprocess

def list_ports():
    try:
        out = subprocess.check_output(
            ["/bin/sh", "-c", "ls /dev/cu.* 2>/dev/null"],
            text=True,
        )
        return [p.strip() for p in out.splitlines() if p.strip()]
    except subprocess.CalledProcessError:
        return []

if __name__ == "__main__":
    ports = list_ports()
    if not ports:
        print("No /dev/cu.* ports found")
    else:
        for p in ports:
            print(p)
