# OS Lab - Processes and fork() in C

This is my lab about processes in C. It uses fork(), wait() and waitpid(),
the exec functions, pipe(), pipelines, and zombie processes.

## Build

    make          # builds all the .c files in src/ and challenges/ into bin/
    make clean    # deletes the bin/ folder

Every .c file has its own main() so each one becomes its own program in bin/.
Run everything from the project root so the paths work.

## Demos (src/)

- fork_demo: shows fork and waitpid. Parent reports the child exit status 42.
  Run: ./bin/fork_demo
- exec_demo: the child turns into ls -l using execlp.
  Run: ./bin/exec_demo
- pipe_demo: the child runs wc -l on piped input and prints 5.
  Run: ./bin/pipe_demo
- pipeline_demo: does ls | grep .c with two children.
  Run: cd src && ../bin/pipeline_demo
- zombie_demo: the child becomes a zombie for 10 seconds, then gets reaped.
  Run: ./bin/zombie_demo

pipeline_demo and ch7 list the current folder, so run them from src/ where
there are actually .c files to find.

## Challenges (challenges/)

- ch1_single_fork: parent prints "child X exited with status 7".
  Run: ./bin/ch1_single_fork
- ch2_multi_child: makes N children, each exits with code index+1.
  Run: ./bin/ch2_multi_child 5
- ch3_exec_ls: child runs ls -la, parent prints after.
  Run: ./bin/ch3_exec_ls
- ch4_exec_worker: runs the worker program with MYVAR=hello.
  Run: ./bin/ch4_exec_worker
- ch5_exec_examples: execl and execv both print "one two".
  Run: ./bin/ch5_exec_examples
- ch6_pipe_sum: child adds 1 to 10 and prints "Sum = 55".
  Run: ./bin/ch6_pipe_sum
- ch7_pipeline: does ls | grep pattern by hand.
  Run: cd src && ../bin/ch7_pipeline "\.c$"
- ch8_wait_nonblock: parent uses WNOHANG so it does not block.
  Run: ./bin/ch8_wait_nonblock
- ch9_zombie: makes a zombie you can see with ps (look below).
  Run: ./bin/ch9_zombie
- ch10_pool: runs at most M workers at a time.
  Run: ./bin/ch10_pool 3 f1 f2 f3 f4 f5

worker.c is the helper program that ch4 runs. The Makefile builds it to bin/worker.

## Zombie screenshot (ch9 and zombie_demo)

Open two terminals.

Terminal A:

    ./bin/ch9_zombie

Terminal B, while it is still sleeping:

    ps -el | grep defunct

You will see the child with the state Z. After the parent calls wait() it is
gone, so run ps again to show that.

## Some things I learned

- You have to check the return of fork() and exec(). Code after a working exec()
  never runs, so if you get there it means exec failed.
- The parent has to close both ends of the pipe or grep waits forever.
- Children use _exit() after fork() instead of exit() so the buffers do not get
  flushed twice.

## Done

I did all 5 demos and all 10 challenges. Everything builds with no warnings.
