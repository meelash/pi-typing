#!/usr/bin/env python3
"""Drives a running QEMU (started by scripts/qemu-run.sh) through its monitor socket.

Usage: qemu-drive.py <build/qemu dir> cmd...
  key:<qemu key names>   e.g. key:ret  key:"shift-a esc"
  text:<ascii text>      types text on the US layout
  wait:<seconds>
  shot:<name>            saves <name>.png in the QEMU directory
"""
import os
import socket
import sys
import time

from PIL import Image

NAMES = {' ': 'spc', '\n': 'ret', ';': 'semicolon', "'": 'apostrophe', ',': 'comma',
         '.': 'dot', '/': 'slash', '-': 'minus'}


def monitor(cmd):
    s = socket.socket(socket.AF_UNIX)
    s.connect("mon.sock")
    s.sendall((cmd + "\n").encode())
    time.sleep(0.2)
    s.close()


def main():
    os.chdir(sys.argv[1])
    for arg in sys.argv[2:]:
        kind, _, val = arg.partition(':')
        if kind == 'key':
            for k in val.split():
                monitor('sendkey ' + k)
                time.sleep(0.15)
        elif kind == 'text':
            for ch in val:
                k = NAMES.get(ch, ch.lower())
                if ch.isupper():
                    k = 'shift-' + k
                monitor('sendkey ' + k)
                time.sleep(0.12)
        elif kind == 'wait':
            time.sleep(float(val))
        elif kind == 'shot':
            monitor('screendump ' + os.path.abspath(val + '.ppm'))
            time.sleep(1.0)
            Image.open(val + '.ppm').save(val + '.png')
            os.remove(val + '.ppm')
            print('shot', os.path.join(sys.argv[1], val + '.png'))


if __name__ == '__main__':
    main()
