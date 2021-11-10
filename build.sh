#!/usr/bin/env bash
gcc -o universal_clipboard.so universal_clipboard.c $(yed --print-cflags) $(yed --print-ldflags)
