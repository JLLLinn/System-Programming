Assignment from CS class, by Jiaxin Lin

This is a simulation of 
Premptive Priority/
Premptive Shortest Job First/
Round Robin with any cycle time/
First Come First Serve
job scheduling.

To compile this MP, run:
make clean
make

To run the helper tester program for part1, run:
./queuetest

To run the simulator, run:
./simulator -c <cores> -s <scheme> <input file>

For example:
./simulator -c 2 -s fcfs examples/proc1.csv

You will find several files:

Programming files:

libpriqueue/libpriqueue.c and libpriqueue/libpriqueue.h: Files related to the priority queue. 

queuetest.c: A small test case to test priority queue, independent of the simulator. 

libscheduler/libscheduler.c and libscheduler/libscheduler.h: Files related to the scheduler.

examples.pl: A perl script of diff runs that tests your program aganist the 54 test output files. This file will output differences between your program and the examples.

Example input files:
examples/proc1.csv
examples/proc2.csv
examples/proc3.csv

Example output files:
examples/proc1-c1-fcfs.out: Sample output of the simulator, using proc1.csv, 1 core, and FCFS scheduling.
examples/proc1-c2-fcfs.out: Sample output of the simulator, using proc1.csv, 2 cores, and FCFS scheduling.