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
#define PERM (S_IRUSR|S_IWUSR)
#define BUFSIZE 24

int timespec2str(char buf[], struct timespec ts) {
        const int bufsize = 31;
        struct tm tm;
        localtime_r(&ts.tv_sec, &tm);
        strftime(buf, bufsize, "%Y-%m-%d %H:%M:%S.", &tm);
        sprintf(buf, "%s%09luZ", buf, ts.tv_nsec);
        return 0;
}

int randomint(int lower, int upper)
{
	int num = (rand() % (upper - lower + 1)) + lower;
	return num;
}

double randomdouble(void)
{
	double dbl = (rand() % RAND_MAX) / (double)RAND_MAX;
	return dbl;
}

// bin_adder adds two integers together and stored the result in place of the first integer
// Arguments passed:
// 1 - name of data file (used to get key to shared memory)
// 2 - number of numbers in shared memory
// 3 - logical id of child
// 4 - logical id of process (this will be 1 to s-1)
// 5 - total number of processs "alive" at one time
int main(int argc,char**argv)
{
	printf("\nHello from consumer");
	printf("\narg1:%s arg2:%s arg3: %s arg4: %s arg5: %s",argv[1],argv[2],argv[3],argv[4],argv[5]);
	key_t key;
	int id=0;
	double *buffer;
	//int numOfNumbers=atoi(argv[2]);
	char *logfile=argv[1];
	int consumerId=atoi(argv[2]);
	//int numProcess=atoi(argv[4]);
	int i;
	char timestr[31];
	struct timespec tpchild;
	FILE *log;
	sem_t *semlock;
	sem_t *semitems;
	sem_t *semslots;
//	int totalProcesses=atoi(argv[5]);

	// Get clock time for log
	if (clock_gettime(CLOCK_REALTIME,&tpchild)==-1)
	{
		perror("Failed to get producer time");
		return 1;
	}
	//printf("\nChild time: %d childId: %d child: %d depth: %d\n",tpchild.tv_nsec,childId,child,depth);
	//timespec2str(timestr, tpchild);
	//printf("\nProducer time: %s",timestr);
	
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

	//for(i=0;i<BUFSIZE;i++)
	//{
	//	printf("Buffer[%d]: %d\n",i,buffer[i]);
	//}

	// Get semaphores
	if((semlock = sem_open("/semlock",0))==SEM_FAILED)
	{
		perror("Failed to open semlock semaphore in consumer");
		return 1;
	}
	if((sem_getvalue(semlock,&i))==-1)
	{
		perror("Failed to get value from semlock semaphore in consumer");
		return 1;
	}
	fprintf(stderr,"semlock: %d\n",i);
	if((semitems = sem_open("/semitems",0))==SEM_FAILED)
	{
		perror("Failed to open semitems semaphore in consumer");
		return 1;
	}
	if((sem_getvalue(semitems,&i))==-1)
	{
		perror("Failed to get value from semitems semaphore in consumer");
		return 1;
	}
	fprintf(stderr,"semitems: %d\n",i);
	if((semslots = sem_open("/semslots",0))==SEM_FAILED)
	{
		perror("Failed to open semslots semaphore in consumer");
		return 1;
	}
	if((sem_getvalue(semslots,&i))==-1)
	{
		perror("Failed to get value from semslots semaphore in consumer");
		return 1;
	}
	fprintf(stderr,"semslots: %d\n",i);

	sleep(randomint(1,10));	

	//entry section
	while(sem_wait(semlock)==-1)
	{
		perror("Failed to lock semlock in consumer");
		return 1;
	}
	//critical section
	fprintf(stderr,"Entering critical section for consumer %d\n",consumerId);
	while(sem_wait(semitems)==-1) {}
	double val = buffer[(int)buffer[21]];
	buffer[22]=buffer[22]+sin(val);
	buffer[23]=(int)buffer[23]+1;
	if ((log=fopen(logfile,"a"))==NULL)
	{
		perror("Failed to open log file from consumer");
		return 1;
	}
	timespec2str(timestr,tpchild);
	//fprintf(log,"\n\ntime:%d pid:%d index:%d depth:%d Value:%d\n",tpchild.tv_nsec,numProcess,first,depth,sharedArea[first]);
	fprintf(stderr,"\n\nConsumer time:%s pid:%d Value:%f",timestr,consumerId,val);
	fclose(log);
	buffer[21] = ((int)buffer[21]+1)%(BUFSIZE-4);
	if(sem_post(semslots)==-1) 
	{
		perror("Failed to post for semslots");
		return 1;
	}
	fprintf(stderr,"\nExiting critical section for consumer %d\n",consumerId);
	//exit section
	if(sem_post(semlock)==-1)
	{
		perror("Failed to unlock semlock");
		return 1;
	}
	
	if((sem_close(semlock))==-1)
	{
		perror("Failed to close semlock semaphore in producer");
		return 1;
	}
	if((sem_close(semitems))==-1)
	{
		perror("Failed to close semitems semaphore in producer");
		return 1;
	}
	if((sem_close(semslots))==-1)
	{
		perror("Failed to close semslots semaphore in producer");
		return 1;
	}

	return 0;
}
