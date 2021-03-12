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
int idstates=0;
double *buffer;
pid_t childpid;
pid_t pids[20];
FILE *logfile;

// This function cleans up shared memory and children
void cleanup()
{
	int i;
	for(i=0;i<BUFSIZE;i++)
	{
		fprintf(stderr,"buffer[%d]: %f\n",i,buffer[i]);
	}
	printf("Estimated average value of sin(x): %f\n",buffer[22]/buffer[23]);
	fprintf(logfile,"Estimated average value of sin(x): %f\n",buffer[22]/buffer[23]);
	char timestr[31];
	struct timespec tpend;
	printf("Clean up started\n");
	fprintf(logfile,"Clean up started\n");
	
	//Clean up shared memory
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
	if(shmdt(state)==-1)
	{
		perror("Failed to deatach from shared memory for states");
		exit(1);
	}
	if(shmctl(idstates,IPC_RMID,NULL)==-1)
	{
		perror("Failed to remove shared memory for states");
		exit(1);
	}
	
	//Clean up semaphores
	if((sem_close(chopstick))==-1)
	{
		perror("Failed to close chopstick semaphore");
		exit(1);
	}
	if((sem_unlink("/chopstick"))==-1)
	{
		perror("Failed to unlink chopstick semaphore");
		exit(1);
	}
	char philname[20];
	for(i=0;i<N;i++)
	{
		sprintf(philname,"philosopher%d",i);
		if((sem_close(philosopher[i]))==-1)
		{
			perror("Failed to close philosopher semaphore");
			exit(1);
		}
		if((sem_unlink(philname))==-1)
		{
			perror("Failed to unlink philosopher semaphore");
			exit(1);
		}
	}
	
	// kill the child processes
	for(i=0;i<20;i++)
	{
		//printf("pids[%d]: %d\n",i,pids[i]);
		if(pids[i]!=0)
		{
			printf("killing child: %d\n",pids[i]);
			fprintf(logfile,"killing child: %d\n",pids[i]);
			kill(pids[i],SIGTERM);
		}
	}
	printf("Clean up complete\n");
	fprintf(logfile,"Clean up complete\n");
	
	if (clock_gettime(CLOCK_REALTIME,&tpend)==-1)
	{
		perror("Failed to get ending time");
		exit(1);
	}
	timespec2str(timestr,tpend);
	printf("\nEnd of program: %s\n",timestr);
	fprintf(logfile,"\nEnd of program: %s\n",timestr);
	fclose(logfile);
}
	

// This function is performed when Ctrl-C is pressed
static void catchctrlcinterrupt(int signo)
{
	char ctrlcmsg[]="Ctrl-C interruption\n";
	int msglen = sizeof(ctrlcmsg);
	write(STDERR_FILENO,ctrlcmsg,msglen);
	write(fileno(logfile),ctrlcmsg,msglen);
	cleanup();
	exit(1);
}

// This function is performed when the timer expires
static void catchtimerinterrupt(int signo)
{
	char timermsg[] = "Timer interrupt after specified number of seconds\n";
	int msglen = sizeof(timermsg);
	write(STDERR_FILENO,timermsg,msglen);
	write(fileno(logfile),timermsg,msglen);
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
	struct timespec tpstart;
	char timestr[31];

	//Open log file for main program
	if ((logfile=fopen("outfile","a"))==NULL)
	{
		perror("Failed to open log file for monitor");
		return 1;
	}
	
	// Get start time for program
	if (clock_gettime(CLOCK_REALTIME,&tpstart)==-1)
	{
		perror("Failed to get starting time");
		return 1;
	}
	timespec2str(timestr, tpstart);
	fprintf(stderr,"Beginning of program: %s\n",timestr);
	fprintf(logfile,"Beginning of program: %s\n",timestr);
	
	// Initialize all pids to 0
	int i;
	for(i=0;i<20;i++)
	{
		pids[i]=0;
	}	

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
		perror("Number of consumers must be greater than number of producers");
		exit(EXIT_FAILURE);
	}

	fprintf(stderr,"Usage: [-o %s] [-p %d] [-c %d] [-t %d] \n",logfilename,m,n,t);
	fprintf(logfile,"Usage: [-o %s] [-p %d] [-c %d] [-t %d] \n",logfilename,m,n,t);
	
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

	// Get shared memory the size of the buffer and initialize memory to zero
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
	for(i=0;i<BUFSIZE;i++)
	{
		buffer[i]=0;
	}
	/*for(i=0;i<BUFSIZE;i++)
	{
		fprintf(stderr,"buffer[%d]: %f\n",i,buffer[i]);
	}*/

	// Get shared memory for the philosopher states and initialize to THINKING
	key_t keystates;
	if((keystates=ftok(".",27)) == -1) {
		perror("Failed to return key for states");
		return 1;
	}
	if((idstates=shmget(keystates,sizeof(int)*N,PERM|IPC_CREAT))==-1)
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
	for(i=0;i<N;i++)
	{
		state[i]=THINKING;
	}
/*	for(i=0;i<N;i++)
	{
		fprintf(stderr,"Philosopher state[%d]: %f\n",i,state[i]);
	}*/
	
	// Get semaphores
	if((chopstick = sem_open("/chopstick",SEM_FLAGS,SEM_PERMS,1))==SEM_FAILED)
	{
		sem_unlink("/chopstick");
		if((chopstick = sem_open("/chopstick",SEM_FLAGS,SEM_PERMS,1))==SEM_FAILED)
		{
			perror("Failed to create chopstick semaphore");
			return 1;
		}
	}
	char philname[20];
	for(i=0;i<N;i++)
	{
		sprintf(philname,"philosopher%d",i);
		if((philosopher[i] = sem_open(philname,SEM_FLAGS,SEM_PERMS,0))==SEM_FAILED)
		{
			sem_unlink(philname);
			if((philosopher[i] = sem_open(philname,SEM_FLAGS,SEM_PERMS,0))==SEM_FAILED)
			{
				perror("Failed to create or open philosopher semaphore");
				return 1;
			}
		}
	}	
	
	// Logic to create children to perform binary tree addition
	char philString[20];
	int totalProducers=1;
	int totalConsumers=1;
	int numProducer=0;
	int numConsumer=0;
	int philNumber=0;
	
			numProducer++;
			philNumber++;
			printf("Incrementing producer count: %d\n",numProducer);
			childpid=fork();
			// Check if child process was not created
			if(childpid==-1)
			{
				perror("Failed to fork a producer");
				return 1;
			}
			if(childpid==0)
			{
				// Arguments have to be passed as strings
				//pids[0]=childpid;
				sprintf(philString,"%d",philNumber-1);
				execl("./producer","producer",logfilename,philString);
				perror("Failed to exec producer");
				return 1;
			}
			pids[0]=childpid;
			sleep(1);
			numConsumer++;
			philNumber++;
			printf("Incrementing consumer count: %d\n",numConsumer);
			childpid=fork();
			if(childpid==-1)
			{	
				perror("Failed to fork a consumer");
				return 1;
			}
			if(childpid==0)
			{
				//pids[1]=childpid;
				sprintf(philString,"%d",philNumber-1);
				execl("./consumer","consumer",logfilename,philString);
				perror("Failed to exec consumer");
				return 1;
			}
			pids[1]=childpid;

		while (wait(NULL)>0)
		{
			// When each child is finished decrement process counter
			//numProcess--;
			//printf("Decrementing process count: %d\n",numProcess);
		}
	
	cleanup();

	return 0;
}
