#define N 20
#define LEFT (i+(N-1))%N
#define RIGHT (i+1)%N
#define THINKING 0
#define HUNGRY 1
#define EATING 2

sem_t *chopstick;
sem_t *philosopher[N];
int *state;

int timespec2str(char buf[], struct timespec ts); 
void test(int i);
void take_chopstick(int i);
void put_chopstick(int i);

