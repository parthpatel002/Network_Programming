# Network Programming Lab Exercise 1

This folder contains my solutions for lab exercise 1 of on-campus Network Programming (IS F462) course. The file description is as follows:

1. `signal.c`: It contains the solution program. 
2. `makefile`: It compiles the code to an executable file `a.out`.
3. `np182_lab1_exercise.pdf`: It describes the problem statement.

## Steps To Run The Code:
This code is implemented in `C` language. To run it, use the commands:
```sh
make
./a.out N K L M
``` 

## Overview:
This lab exercise is based on the concept of forking a new process and sending signals from one process to other. The program lists PIDs of all the newly created processes (using `fork()`) in `PIDs.txt`.

