#include<stdio.h>
#include<stdlib.h>
#include<sys/sem.h>
#include<sys/shm.h>
#include<sys/wait.h>
#include<sys/stat.h>
#include<time.h>
#include<sys/time.h>
#include<math.h>
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

// Consumer: consumers random number from a shared buffer and accumulates
//  the sum of the sine values so that an average can be estimated
// Arguments passed:
// 1 - name of data file
// 2 - philosopher number of process
int main(int argc,char**argv)
{
	//printf("\nHello from consumer");
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
		perror("Failed to get consumer time");
		return 1;
	}
	//printf("\nChild time: %d childId: %d child: %d depth: %d\n",tpchild.tv_nsec,childId,child,depth);

	//Use current time as seed for random generator
	srand(time(0));

	// Get access to shared memory set up by the parent
	if((key=ftok(".",13))==-1)
	{
		perror("Consumer failed to get key");
		return 1;
	}
	if((id=shmget(key,sizeof(double)*BUFSIZE,PERM))==-1)
	{
		perror("Failed to create shared memory in consumer");
		return 1;	
	}
	if((buffer=(double*)shmat(id,NULL,0))==(void*)-1)
	{
		perror("Failed to attach to shared memory in consumer");
		if(shmctl(id,IPC_RMID,NULL)==-1)
		{
			perror("Failed to remove memory segment in consumer");
		}
		return 1;
	}
	
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
	/*for(i=0;i<BUFSIZE;i++)
	{
		printf("Buffer[%d]: %d\n",i,buffer[i]);
	}*/

	// Get semaphores
	if((chopstick = sem_open("/chopstick",0))==SEM_FAILED)
	{
		perror("Failed to open chopstick semaphore in consumer");
		return 1;
	}
	char philname[20];
	for(i=0;i<N;i++)
	{
		sprintf(philname,"philosopher%d",i);
		if((philosopher[i] = sem_open(philname,0))==SEM_FAILED)
		{
			perror("Failed to open philosopher semaphore in consumer");
			return 1;
		}
	}

	while(1)
	{
		sleep(randomint(1,10));	
		//entry section
		take_chopstick(philId);
		//critical section
		if ((log=fopen(logfile,"a"))==NULL)
		{
			perror("Failed to open log file from consumer");
			return 1;
		}
		fprintf(stderr,"Entering critical section for consumer/philosopher %d\n",philId);
		fprintf(log,"Entering critical section for consumer/philosopher %d\n",philId);
		double val = buffer[(int)buffer[21]];
		//If number is available for consuming, else do nothing
		if(val!=0.0)
		{
			buffer[22]=buffer[22]+sin(val);
			buffer[23]=(int)buffer[23]+1;
			timespec2str(timestr,tpchild);
			fprintf(stderr,"Consumer time:%s pid:%d Value:%f\n",timestr,philId,val);
			fprintf(log,"Consumer time:%s pid:%d Value:%f\n",timestr,philId,val);
			buffer[(int)buffer[21]]=0.0;
			buffer[21] = ((int)buffer[21]+1)%(BUFSIZE);
		}
		fprintf(stderr,"Exiting critical section for consumer/philosopher %d\n",philId);
		fprintf(log,"Exiting critical section for consumer/philosopher %d\n",philId);
		fclose(log);
		//exit section
		put_chopstick(philId);
	}

	if((sem_close(chopstick))==-1)
	{
		perror("Failed to close chopstick semaphore in consumer");
		return 1;
	}
	return 0;
}
