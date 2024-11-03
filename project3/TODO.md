# TODO

- [X] main: Thread pool
    - [X] Number of threads on argument `-t`
- [ ] main: Accept connections into buffer
    - [ ] Buffer size on argument `-b`
    - [ ] Place socket/descriptor into buffer
- [ ] producer-consumer
    - [ ] main blocks if buffer is full
    - [ ] worker wait if buffer is empty
- [ ] worker: scheduling
    - [ ] Wake when request in queue
    - [ ] Handle the oldest request in queue

# Implementation notes

## dthread

Use dthread instead of pthread, with the same methods:

create, detach, mutex\_lock, mutex\_unlock, cond\_wait, cond\_signal

Initialize with pthread variables:

MUTEX\_INITIALIZER, COND\_INITIALIZER

## Project structure

Modification should be in `gunrock.cpp` and `FileService.cpp`

## Buffer

Buffer stores sockets. A circular queue with a head and tail indices should
work.
