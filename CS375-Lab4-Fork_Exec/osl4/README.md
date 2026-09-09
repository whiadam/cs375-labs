# CS375 Lab 4 - Process Creation Using Fork and Exec in C

This lab uses fork() and exec() to make processes and run programs.
The last exercise is a small shell that runs commands the user types.

## Build

    make          # builds all the .c files into bin/
    make clean    # deletes bin/

You can also compile one file at a time like the lab shows:

    gcc ex1_basic_fork.c -o ex1_basic_fork
    ./ex1_basic_fork

## Exercises

- ex1_basic_fork: the parent and the child each print a message with their PID.
  Run: ./bin/ex1_basic_fork
- ex2_grandchild: the child makes its own child, so there is a parent, a child,
  and a grandchild.
  Run: ./bin/ex2_grandchild
- ex3_two_children: the parent makes two children.
  Run: ./bin/ex3_two_children
- ex4_exec_ls: one child runs ls -l with execlp, and the parent waits for it.
  Run: ./bin/ex4_exec_ls
- ex5_shell: a simple shell. Type a command and it runs it. Type exit to quit.
  Run: ./bin/ex5_shell

## Shell example

    myshell> ls
    myshell> pwd
    myshell> exit

## Notes

- The starter code for exercise 3 called fork() twice in a row, which really
  makes four processes, not three. I only fork the second time inside the parent
  branch so there are exactly two children.
- My shell splits the line into words, so it can run commands with options like
  ls -l, not just single words. It also stops if you press Ctrl+D.
- The exec lines are followed by an error print. That code only runs if exec
  failed, because a working exec replaces the whole program.
