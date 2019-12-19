#!/bin/bash
./cc0 -s in.c0 -o in.s0
./c0-vm-cpp -a in.s0 in.o0
./c0-vm-cpp -r in.o0