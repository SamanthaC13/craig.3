#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <semaphore.h>
#define N 20
#define LEFT (i+(N-1))%N
#define RIGHT (i+1)%N
#define THINKING 0
#define HUNGRY 1
#define EATING 2

sem_t *chopstick;
sem_t *philosopher[N];
int *state;


int timespec2str(char buf[], struct timespec ts) {
        const int bufsize = 31;
        struct tm tm;
        localtime_r(&ts.tv_sec, &tm);
        strftime(buf, bufsize, "%Y-%m-%d %H:%M:%S.", &tm);
        sprintf(buf, "%s%09luZ", buf, ts.tv_nsec);
        return 0;
}

void test(int i)
{
	if(state[i]==HUNGRY && state[LEFT]!=EATING && state[RIGHT]!=EATING)
	{
		state[i]=EATING;
		printf("Philosopher %d takes chopstick %d and %d\n",i,LEFT,i);
		printf("Philosopher %d eats\n",i);
		sem_post(philosopher[i]);
	}
}

void take_chopstick(int i)
{
	sem_wait(chopstick);
	state[i]=HUNGRY;
	printf("Philosopher %d is hungry\n",i);
	test(i);
	sem_post(chopstick);
	sem_wait(philosopher[i]);
}

void put_chopstick(int i)
{
	sem_wait(chopstick);
	state[i]=THINKING;
	printf("Philosopher %d puts down chopstick %d and %d\n",i,LEFT,i);
	printf("Philosopher %d thinks\n",i);
	test(LEFT);
	test(RIGHT);
	sem_post(chopstick);
}
