# Concurrent Bank Operations — C / pthreads Coursework

A historical operating-systems exercise modeling concurrent bank operations
with producer/consumer threads, a shared queue, mutexes and condition variables.
The interesting boundary is synchronization of shared operations and balances;
this is not financial software.

[Engineering portfolio](https://github.com/miguelcastu/miguelcastu) · [Project directory](https://github.com/miguelcastu/miguelcastu/blob/main/PROJECTS.md)

## Source map

- [bank.c](bank.c): argument handling, operation parsing and thread coordination.
- [queue.c](queue.c) and [queue.h](queue.h): queue implementation and contract.
- [operations.txt](operations.txt): sample input; inspect it before selecting parameters.
- [Makefile](Makefile): GCC/pthread build. Original checker, outputs and historical
  binary/object files remain preserved.

## Build and execution contract

On Linux/WSL with GCC, Make and pthreads:

```shell
make
```

`bank` expects five positional arguments: input file, ATM thread count, worker
thread count, maximum account count and buffer size. Select positive counts
consistent with the input format after reading `main()`; the original argument
validation is incomplete and this audit has not verified a safe parameter set.
Do not run the program with missing arguments. A Windows build, race-free behavior
and all interleavings are not established by source inspection.
