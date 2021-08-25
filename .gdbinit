set environment LD_LIBRARY_PATH=contrib/build/cmocka/src/
handle SIGUSR2 pass nostop
set print elements 0
set follow-fork-mode child
set detach-on-fork on
add-auto-load-safe-path /usr/local/go/src/runtime/runtime-gdb.py
set breakpoint pending on
