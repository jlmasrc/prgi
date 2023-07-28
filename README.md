# prgi
C library for progress indicators with low overhead and multithread support

## Introduction

This library provides timed progress indicators at the terminal for single- or multi-threaded C programs with high CPU usage (for example, numeric software). The following progress indicators are supported: 

- Progress bar, with optional embedded text (for example, percentage progress)
- Percentage progress
- Throbbers
- Elapsed time
- Estimated remaining time
- Instantaneous program speed
- Mean program speed

The progress indicators are recalculated e shown with fixed periodcity independent of program or computer speed, thus allowing very low overhead, well suited to high CPU usage programs. The library consists only in the C source and header file, allowing simple integration with the program by copying those 2 files to the user tree.

## Rationale

Usually, a task that demands a progress indicator has the form
```
for(n = 0; n < N; n++) {
    do_something();
}
```
A naive solution to this problem would be
```
for(n = 0; n < N; n++) {
    do_something();
    /* Print the percentage progress */
    printf("\r%d%%", 100*n/N);
}
```
However, if N is very large and do_something() takes very little processing, the task will spend most of the time printing the progress and not doing real processing in do_something().

A better solution could be
```
for(n = 0; n < N; n++) {
    do_something();
    if(n % T == 0) printf("\r%d%%", 100*n/N);
}
```
Then, the progress indicator would trigger only once each T loops (for example, T = 1000000). There are some problems with this approach:
- The number T must be adjusted taking into account N and how much processing time do_something() needs.
- The number T may need to be readjusted when a new machine arrives.
- The number T should be adjusted also taking into account the machine availability in case other CPU intensive processes are also present.

The library prgi (progress indicators) sove those problems by showing the progress indicators at fixed, configurable time intervals in a self-adjustable way that is task, machine and availability independent. This is done while maintaining a very low (almost unnoticeable) overhead on the program, supporting a feature-rich set of progress indicators and being compatible with multi-threaded programs.

## Usage

The documentation of functions and variables of the prgi library is in the file prgi.h. There are a handful of examples that should make usage clear (starting with example_tutorial.c).
