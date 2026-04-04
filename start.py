import os
import time

def get_timestamps(root):
    times = {}
    for base, dirs, files in os.walk(root):
        for f in files:
            if f.endswith((".cpp", ".hpp")):
                path = os.path.join(base, f)
                times[path] = os.path.getmtime(path)
    return times

last = get_timestamps("src")

while True:
    time.sleep(1)
    now = get_timestamps("src")
    if now != last:
        print("Change detected!")
        last = now
