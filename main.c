#include<stdlib.h>
#include<stdio.h>
#include<unistd.h>
#include<sys/ipc.h>
#include<sys/shm.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<time.h>
#include<sys/stat.h>
#include<signal.h>
#include<sys/time.h>
#include<semaphore.h>
#include<fcntl.h>
#include<errno.h>
#include "libmonitor.h"
#define PERM (S_IRUSR|S_IWUSR)
#define BUFSIZE 24
#define SEM_PERMS (mode_t)(S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)
#define SEM_FLAGS (O_CREAT|O_EXCL)

// global variables
int id=0;
double *buffer;
sem_t *semlock;
sem_t *semitems;
sem_t *semslots;
pid_t childpid;

// This function cleans up shared memory and children
void cleanup()
{
	char timestr[31];
	struct timespec tpend;
	printf("Clean up started\n");
	if(shmdt(buffer)==-1)
	{
		perror("Failed to deatach from shared memory");
		exit(1);
	}
	if(shmctl(id,IPC_RMID,NULL)==-1)
	{
		perror("Failed to remove shared memory");
		exit(1);
	}
	if((sem_close(semlock))==-1)
	{
		perror("Failed to close semlock semaphore");
		exit(1);
	}
	if((sem_unlink("/semlock"))==-1)
	{
		perror("Failed to unlink semlock semaphore");
		exit(1);
	}
	if((sem_close(semitems))==-1)
	{
		perror("Failed to close semitems semaphore");
		exit(1);
	}
	if((sem_unlink("/semitems"))==-1)
	{
		perror("Failed to unlink semitems semaphore");
		exit(1);
	}
	if((sem_close(semslots))==-1)
	{
		perror("Failed to close semslots semaphore");
		exit(1);
	}
	if((sem_unlink("/semslots"))==-1)
	{
		perror("Failed to unlink semslots semaphore");
		exit(1);
	}
	//kill(childpid,SIGTERM);
	printf("Clean up complete\n");
	if (clock_gettime(CLOCK_REALTIME,&tpend)==-1)
	{
		perror("Failed to get ending time");
		exit(1);
	}
	timespec2str(timestr,tpend);
	printf("\nEnd of program: %s\n",timestr);
}

// This function is performed when Ctrl-C is pressed
static void catchctrlcinterrupt(int signo)
{
	char ctrlcmsg[]="Ctrl-C interruption\n";
	int msglen = sizeof(ctrlcmsg);
	write(STDERR_FILENO,ctrlcmsg,msglen);
	cleanup();
	exit(1);
}

// This function is performed when the timer expires
static void catchtimerinterrupt(int signo)
{
	char timermsg[] = "Timer interrupt after specified number of second\n";
	int msglen = sizeof(timermsg);
	write(STDERR_FILENO,timermsg,msglen);
	cleanup();
	exit(1);
}

// This function is called to setup the timer which will be set by the variable t
static int setuptimer(int t)
{
	struct itimerval value;
	value.it_interval.tv_sec = t;
	value.it_interval.tv_usec = 0;
	value.it_value = value.it_interval;
	return (setitimer(ITIMER_REAL,&value,NULL));
}

int main (int argc,char* argv[])
{
	int option=0;
	char *logfilename="logfile"; 
	int m=2;
	int n=6;
	int t=100;
	int num=0;
	int numOfNumbers=0;
	FILE *fptr;
	struct timespec tpstart;
	char timestr[31];

	// Get start time for program
	if (clock_gettime(CLOCK_REALTIME,&tpstart)==-1)
	{
		perror("Failed to get starting time");
		return 1;
	}
	timespec2str(timestr, tpstart);
	fprintf(stderr,"Beginning of program: %s\n",timestr);
	
	// Get program arguments 
	while((option=getopt(argc,argv, "ho:p:c:t:"))!=-1)
	{	
		switch(option)
		{
			case'h':
				printf("help");
				break;
			case'o':
				logfilename=optarg;
				break;
			case'p':
				m=atoi(optarg);
				break;
			case'c':
				n=atoi(optarg);
				break;
			case't':
				t=atoi(optarg);
				break;
		}
	}
	// Check to see if time value is valid
	if(t<=0)
	{
		perror("Invalid time entered");
		exit(EXIT_FAILURE);
	}
	// Check to see if number of producers is valid
	if(m<=0)
	{
		perror("Invalid number of producers entered");
		exit(EXIT_FAILURE);
	}
	// Check to see if number of consumers is valid
	if(n<=0)
	{
		perror("Invalid number of consumers entered");
		exit(EXIT_FAILURE);
	}
	// Check if number of consumers is greater than number of producers
	if(m>=n)
	{
		perror("Number of producers must be greater than number of consumers");
		exit(EXIT_FAILURE);
	}
	// Set a hard limit of 20 processes even if more is entered
/*	if(s>20)
	{
		s=20;
		fprintf(stderr,"Number of processes is being limited to 20");
	}
*/
	fprintf(stderr,"Usage: [-o %s] [-p %d] [-c %d] [-t %d] \n",logfilename,m,n,t);
/*	
	// Set up the timer
	if(setuptimer(t)==-1)
	{
		perror("Failed to set up timer");
		return 1;
	}
	// Set up timer interrupt
	signal(SIGALRM, catchtimerinterrupt);
	// Set up Ctrl-C interrupt
	signal(SIGINT, catchctrlcinterrupt);

	// Open input file of numbers
	if((fptr=fopen(argv[optind],"r"))==NULL){
		perror("Failed to open file");
		return 1;
	}
	// Read input file to see how many numbers are in file
	// This number is assumed to be a power of 2
	while((fscanf(fptr,"%d",&num))==1)
	{
		numOfNumbers++;
		fprintf(stderr,"%d\n",num);
	}
	// Chick to see if any numbers are in file
	if(numOfNumbers==0)
	{
		perror("File has no contents");
		return 1;
	}
	fclose(fptr);
*/	
	// Get shared memory the size of the buffer
	key_t key;
	if((key=ftok(".",13)) == -1) {
		perror("Failed to return key");
		return 1;
	}
	if((id=shmget(key,sizeof(double)*BUFSIZE,PERM|IPC_CREAT))==-1)
	{
		perror("Failed to create shared memory segment");
		return 1;
	}
	if((buffer=(double*)shmat(id,NULL,0))==(void*)-1)
	{
		perror("Failed to attach shared memory segment");
		if(shmctl(id,IPC_RMID,NULL)==-1)
		{	
			perror("Failed to remove shared memory segment");
		}
		return 1;
	}
	int i;
	for(i=0;i<BUFSIZE;i++)
	{
		buffer[i]=0;
	}
	for(i=0;i<BUFSIZE;i++)
	{
		fprintf(stderr,"buffer[%d]: %f\n",i,buffer[i]);
	}

	// Get semaphores
	if((semlock = sem_open("/semlock",SEM_FLAGS,SEM_PERMS,1))==SEM_FAILED)
	{
		sem_unlink("/semlock");
		if((semlock = sem_open("/semlock",SEM_FLAGS,SEM_PERMS,1))==SEM_FAILED)
		{
			perror("Failed to create semlock semaphore");
			return 1;
		}
	}
	if((sem_getvalue(semlock,&i))==-1)
	{
		perror("Failed to get value from semlock semaphore");
		return 1;
	}
	fprintf(stderr,"semlock: %d\n",i);
	if((semitems = sem_open("/semitems",SEM_FLAGS,SEM_PERMS,0))==SEM_FAILED)
	{
		sem_unlink("/semitems");
		if((semitems = sem_open("/semitems",SEM_FLAGS,SEM_PERMS,0))==SEM_FAILED)
		{
			perror("Failed to create or open semitems semaphore");
			return 1;
		}
	}
	if((sem_getvalue(semitems,&i))==-1)
	{
		perror("Failed to get value from semitems semaphore");
		return 1;
	}
	fprintf(stderr,"semitems: %d\n",i);
	if((semslots = sem_open("/semslots",SEM_FLAGS,SEM_PERMS,BUFSIZE-4))==SEM_FAILED)
	{
		sem_unlink("/semslots");
		if((semslots = sem_open("/semslots",SEM_FLAGS,SEM_PERMS,BUFSIZE-4))==SEM_FAILED)
		{
			perror("Failed to create or open semslots semaphore");
			return 1;
		}
	}
	if((sem_getvalue(semslots,&i))==-1)
	{
		perror("Failed to get value from semslots semaphore");
		return 1;
	}
	fprintf(stderr,"semslots: %d\n",i);



/*
	// Open file to read numbers
	if((fptr=fopen(argv[optind],"r"))==NULL){
		perror("Failed to open file");
		return 1;
	}
	// Read numbers are store them in shared memory
	int i=0;
	while((fscanf(fptr,"%d",&num))==1)
	{
		sharedArea[i]=num;
		i++;	
	}
	fclose(fptr);
*/	
	// Logic to create children to perform binary tree addition
	char numString[20];
	char iString[20];
	char pString[20];
	char sString[20];
	sprintf(numString,"%d",numOfNumbers);
	int totalProducers=1;
	int totalConsumers=1;
	int numProducer=0;
	int numConsumer=0;


	// Loop for the number of children needed to perform task
	// This loops from the number needed down to 0
/*	while (i>0)
	{
		// Loop until maximum child processes are created or
		// all child processes have been created
		while (numProcess<totalProcesses && i>0)
		{
*/			// Increment counter for number of processes
			numProducer++;
			printf("Incrementing producer count: %d\n",numProducer);
			childpid=fork();
			// Check if child process was not created
			if(childpid==-1)
			{
				perror("Failed to fork");
				//break;
				return 1;
			}
			// Perform child logic
			if(childpid==0)
			{
				// Execute bin_adder process passing in needed arguments
				// Arguments have to be passed as strings
				sprintf(iString,"%d",i);
				sprintf(pString,"%d",numProducer);
				sprintf(sString,"%d",totalProducers);
				//execl("./bin_adder","bin_adder",argv[optind],numString,iString,pString,sString,NULL);
				execl("./producer","producer",logfilename,pString);
				perror("Failed to exec producer");
				return 1;
			}
			// Perform parent logic
			i--;
			sleep(1);
			numConsumer++;
			printf("Incrementing consumer count: %d\n",numConsumer);
			childpid=fork();
			if(childpid==-1)
			{	
				perror("Failed to fork");
				return 1;
			}
			if(childpid==0)
			{
				sprintf(pString,"%d",numConsumer);
				execl("./consumer","consumer",logfilename,pString);
				perror("Failed to exec consumer");
				return 1;
			}
//		}
		// Wait for child process to finish
		while (wait(NULL)>0)
		{
			// When each child is finished decrement process counter
			//numProcess--;
			//printf("Decrementing process count: %d\n",numProcess);
		}
//	}
//	// Parent process is complete and answer is found
//	printf("Final answer: %d\n",sharedArea[0]);
//	
	for(i=0;i<BUFSIZE;i++)
	{
		fprintf(stderr,"buffer[%d]: %f\n",i,buffer[i]);
	}
	if((sem_getvalue(semlock,&i))==-1)
	{
		perror("Failed to get value from semlock semaphore");
		return 1;
	}
	fprintf(stderr,"semlock: %d\n",i);
	if((sem_getvalue(semitems,&i))==-1)
	{
		perror("Failed to get value from semitems semaphore");
		return 1;
	}
	fprintf(stderr,"semitems: %d\n",i);
	if((sem_getvalue(semslots,&i))==-1)
	{
		perror("Failed to get value from semslots semaphore");
		return 1;
	}
	fprintf(stderr,"semslots: %d\n",i);


	cleanup();

	return 0;
}
