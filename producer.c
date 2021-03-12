#include<stdio.h>
#include<stdlib.h>
#include<sys/sem.h>
#include<sys/shm.h>
#include<sys/wait.h>
#include<sys/stat.h>
#include<time.h>
#include<sys/time.h>
#include <math.h>
#include<semaphore.h>
#include "libmonitor.h"
#define PERM (S_IRUSR|S_IWUSR)
#define BUFSIZE 24

//Function to produce random integers
int randomint(int lower, int upper)
{
	int num = (rand() % (upper - lower + 1)) + lower;
	return num;
}

//Function to produce random doubles
double randomdouble(void)
{
	double dbl = (rand() % RAND_MAX) / (double)RAND_MAX;
	return dbl;
}
  
// Producer: produced random numbers and puts them in available slots in the shared buffer
// Arguments passed:
// 1 - name of data file 
// 2 - philosopher number of process
int main(int argc,char**argv)
{
	//printf("\nHello from producer");
	key_t key;
	int id=0,idstates=0;
	double *buffer;
	char *logfile=argv[1];
	int philId=atoi(argv[2]);
	int i;
	char timestr[31];
	struct timespec tpchild;
	FILE *log;

	// Get clock time for log
	if (clock_gettime(CLOCK_REALTIME,&tpchild)==-1)
	{
		perror("Failed to get producer time");
		return 1;
	}
	//printf("\nChild time: %d childId: %d child: %d depth: %d\n",tpchild.tv_nsec,childId,child,depth);

	//Use current time as seed for random generator
	srand(time(0));

	// Get access to shared memory set up by the parent
	if((key=ftok(".",13))==-1)
	{
		perror("Producer failed to get key");
		return 1;
	}
	if((id=shmget(key,sizeof(double)*BUFSIZE,PERM))==-1)
	{
		perror("Failed to create shared memory in producer");
		return 1;	
	}
	if((buffer=(double*)shmat(id,NULL,0))==(void*)-1)
	{
		perror("Failed to attach to shared memory in producer");
		if(shmctl(id,IPC_RMID,NULL)==-1)
		{
			perror("Failed to remove memory segment in producer");
		}
		return 1;
	}
	/*for(i=0;i<BUFSIZE;i++)
	{
		printf("Buffer[%d]: %d\n",i,buffer[i]);
	}*/

	// Get access to philosopher states set up by the monitor
	key_t keystates;
	if((keystates=ftok(".",27)) == -1) {
		perror("Failed to return key for states");
		return 1;
	}
	if((idstates=shmget(keystates,sizeof(int)*N,PERM))==-1)
	{
		perror("Failed to create shared memory segment for states");
		return 1;
	}
	if((state=shmat(idstates,NULL,0))==(void*)-1)
	{
		perror("Failed to attach shared memory segment for states");
		if(shmctl(idstates,IPC_RMID,NULL)==-1)
		{	
			perror("Failed to remove shared memory segment for states");
		}
		return 1;
	}

	// Get semaphores
	if((chopstick = sem_open("/chopstick",0))==SEM_FAILED)
	{
		perror("Failed to open chopstick semaphore in producer");
		return 1;
	}
	char philname[20];
	for(i=0;i<N;i++)
	{
		sprintf(philname,"philosopher%d",i);
		if((philosopher[i] = sem_open(philname,0))==SEM_FAILED)
		{
			perror("Failed to open philosopher semaphore in producer");
			return 1;
		}
	}	
	
	while(1)
	{
		//entry section
		take_chopstick(philId);
		//critical section
		sleep(1);
		if ((log=fopen(logfile,"a"))==NULL)
		{
			perror("Failed to open log file from producer");
			return 1;
		}
		fprintf(stderr,"Entering critical section for producer/philosopher %d\n",philId);
		fprintf(log,"Entering critical section for producer/philosopher %d\n",philId);
		//if slot is available for use, else do nothing
		if(buffer[(int)buffer[20]]==0.0)
		{
			buffer[(int)buffer[20]]=randomdouble();
			timespec2str(timestr,tpchild);
			fprintf(stderr,"Producer time:%s pid:%d Value:%f\n",timestr,philId,buffer[(int)buffer[20]]);
			fprintf(log,"Producer time:%s pid:%d Value:%f\n",timestr,philId,buffer[(int)buffer[20]]);
			buffer[20] = ((int)buffer[20]+1)%(BUFSIZE-4);
		}
		fprintf(stderr,"Exiting critical section for producer %d\n",philId);
		fprintf(log,"Exiting critical section for producer %d\n",philId);
		fclose(log);
		//exit section
		put_chopstick(philId);
		sleep(randomint(1,5));
	}
	
	if((sem_close(chopstick))==-1)
	{
		perror("Failed to close semitems semaphore in producer");
		return 1;
	}
	return 0;
}
