#!/bin/sh

x86_64-w64-mingw32-gcc -shared patch.c minhook/src/buffer.c minhook/src/hook.c minhook/src/trampoline.c minhook/src/hde/hde64.c \
                       -Os -static-libgcc -o obsidian.dll
strip --strip-all -R .note -R .comment obsidian.dll

x86_64-w64-mingw32-gcc -Wl,--subsystem,windows launch.c -Os -static -static-libgcc -lshlwapi -o obsidian.exe
strip --strip-all -R .note -R .comment obsidian.exe

rm -f obsidian.zip
zip -9 obsidian.zip README.txt obsidian.dll obsidian.exe

rm -f obsidian-src.zip
zip -9 obsidian-src.zip README.txt obsidian.dll obsidian.exe patch.c launch.c minhook/src/buffer.c minhook/src/hook.c \
       minhook/src/trampoline.c minhook/src/hde/hde64.c build.sh clean.sh
