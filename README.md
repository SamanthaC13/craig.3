# craig.3
CS4760 Assignment 3

This program is compiled using the make command. The executable that runs this program is named monitor. This program solves the producer consumer problem using the the dining philosopher problem and monitors.
The dining phiolospher's problem is implmented in the monitor. Each process spawned from the parent either producer or consumer is considered a philospher. Before the critical section in both the producer and the concumer
the philosopher process must go to the moniter and make sure it can eat and that the chopsticks on either side of that philosopher process is ok to be used. Each child process that is spawned from the parent either producer or 
consumer is run in an infinte loop. The child processes are only stoped by the time innterupt or the signal intrupt. When that inturpt happens all children are killed and shared memory and semaphore are unlinked and destoryed. 

The problems encountered during this program were mainly in the concepts. It was hard to understand how to combine the idea of solving the producer consumer problem with monitors while also incorporating the dinning philosophers problem.

