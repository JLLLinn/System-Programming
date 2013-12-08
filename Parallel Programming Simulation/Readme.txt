This program is a course assignment. It imitates the common "make" utility. 

The Parse function will list dependencies and commands to run for different target values. "parmake" uses this parse function to store the work that needs to be done. Then using a fixed pool of threads, you will make sure that all commands are executed after their dependencies. When there is less work to be done than there are threads, the remaining idle threads should sleep.

All threads will be as busy as possible and there is no busy waiting, threads are blocked when no job is available, and "waked" when a job is available.

To compile, run the following commands from a command prompt on a Linux machine:
%> make clean 
%> make

To run the executable,
%> ./parmake [ -f makefile ] [ -j threads] [ targets ]

For example:
%> ./parmake -f testfile4 -j 2

This should generate the same output as:

%> make -f testfile4 -j 2
 === OR ===
%> make -s -f testfile4 -j 2