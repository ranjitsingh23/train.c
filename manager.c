//AS3 Saksham Grover & Piyush
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <time.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <limits.h>

#define SEQLENGTH 100
#define SEMAPHOREKEY 2058

#define MUTX_1_1 5
#define MUTX_2 4
#define NORTH 0
#define WEST 1
#define SOUTH 2
#define EAST 3


char *dishaName[] = {"North", "West", "South", "East", "Junction"}; 
//direction defined
int semaphoreid1; 
//making semaphore id
FILE *flName; 
int zerochild = 0; 
char seq[SEQLENGTH]; //the sequence 
int processIdArray[SEQLENGTH]; //process identification array which shows includes the sequence length
int n; 
//n shows the number of train

int semWait(int semaphoreid1, int subsemaphore)
{
	struct sembuf sop;
	sop.sem_num = subsemaphore;
	sop.sem_op = -1;
	sop.sem_flg = 0;
	return semop(semaphoreid1, &sop, 1);
}

int semaphoreSignal(int semaphoreid1, int subsemaphore)
{
	struct sembuf sop;
	sop.sem_num = subsemaphore;
	sop.sem_op = 1;
	sop.sem_flg = 0;
	return semop(semaphoreid1, &sop, 1);
}

int getdirc(char d)
 //to get direction
{
	int dirc;
	switch (d)
	{
	case 'N' : dirc = NORTH;
		break;
	case 'W' : dirc = WEST;
		break;
	case 'E' : dirc = EAST;
		break;
	case 'S' : dirc = SOUTH;
		break;
	}
	return dirc;
}

void rdMatrixFile() 
//make it read data from file
{
	flName = fopen("matrix.txt", "r");
	if (flName == NULL)
	{
		printf("Error\n");
		exit(1);
	}
	for (int a = 0; a < n; a++)
	{
		for (int j = 0; j < 4; j++)
		{
			int numb = 0;
			fscanf(flName, "%d", &numb);
			printf("%d ", numb);
		}
		printf("\n");
	}
	fclose(flName);
}


void DeadlockCnt(int sig)
{
	pid_t prid; //processid of the children process
	int status;
	//create while loop for children process
	while (1) {

		//get child process ID if there is any
		prid = waitpid(-1, &status, WNOHANG);
		if (prid == -1) {
			// interruption EINTR
			if (errno == EINTR) {
				continue;
			}
			break;
		}
		else if (prid == 0) {
			//if no process it will end
			break;
		}
		else if (prid > 0)
			zerochild++;
	}
}

int rdSequenceFile() 
//made it read data from seq file
{
	FILE *flName = fopen("sequence.txt", "r");
	if(flName==NULL)
	{
		printf("Error opening sequence file.\n");
		exit(1);
	}
	int a = 0;
	char chr;
	//character checking one by one
	while ((chr = (char)fgetc(flName) ) != EOF)
	{
		seq[a++] = chr;
	}
	fclose(flName);
	return a;
}

void printCycle(int par[], int v, int a) //printing cycle
{
	printf("System Deadlocked\n");
	int p = v;
	par[a] = v;
	//vertices assigned
	while(p >= n) 
	   p = par[p];
	int start = p;
	int cycle[n]; 
	int k = 0;
	do
	{
		int wttrain = par[par[p]];
		cycle[k++] = p; 
		p = wttrain;
	}while(p != start);
	k--;
	int j = k;
	while(k > 0)
	{
		p = cycle[k-1];
		int wttrain = cycle[k];
		printf("train %d from %s is waiting train %d from %s",processIdArray[wttrain],dishaName[getdirc(seq[wttrain])],
			processIdArray[p],dishaName[getdirc(seq[p])]);
		printf(" ---> ");
		k--;
	}
	int wttrain = cycle[0];
	p = cycle[j];
	printf("train %d from %s is waiting train %d from %s\n",processIdArray[wttrain],dishaName[getdirc(seq[wttrain])],
			processIdArray[p],dishaName[getdirc(seq[p])]);
}

void init() //initializing the matrix
{
	flName = fopen("matrix.txt", "w");
	if (flName == NULL)
	{
		printf("Error creating file\n");
		exit(1);
	}
	for (int a = 0; a < n; a++)
	{
		for (int j = 0; j < 4; j++)
		{
			fprintf(flName, "%d ", 0);
		}
		fprintf(flName, "\n");
	}
	fclose(flName);
}

int cycleRun(int gph[][SEQLENGTH + 4], int *vist, int*par)
{
	int a;
	for (a = 0; a < n + 4; a++)
	{
		vist[a] = 0;
		par[a] = a;
	}
	for (a = 0; a < n + 4; a++)
	{
		//calling function
		if (workCycle(a, gph, vist, par))
			return 1;
	}
	return 0;
}

void findDeadlock() //function to detect deadlock
{
	int a, j;
        semWait(semaphoreid1, MUTX_1_1); //wait for a train to pass
	int gph[SEQLENGTH + 4][SEQLENGTH + 4];
	for (a = 0; a < n + 4; a++)
	{
		for (j = 0; j < n + 4; j++)
		{
			gph[a][j] = 0; //initialize the gph
		}
	}
	flName = fopen("matrix.txt", "r");
	for (a = 0; a < n; a++)
	{
		for (j = 0; j < 4; j++)
		{
			int val = 0; //value in the sequence
			fscanf(flName, "%d", &val);
			if (val == 1)
			{
				gph[a][n + j] = 1;
			}
			else if (val == 2)
			{
				gph[n + j][a] = 1;
			}
		}
	}
	fclose(flName);
	int vist[n + 4], par[n + 4];
	if (cycleRun(gph, vist, par))
	{
		signal(SIGTERM, SIG_IGN);//child process ignores the signal
		killpg(0, SIGTERM); //send signal to all children
		exit(0);
	}
	semaphoreSignal(semaphoreid1, MUTX_1_1);
}

//working cycle function
int workCycle(int v, int gph[][SEQLENGTH + 4], int *vist,int *par)
{
	int a;
	if (!vist[v]) 
	{
		vist[v] = 1; //to check reccurance occuring
		for (a = 0; a < n + 4; a++)
		{
			if (gph[v][a] == 1)
			{
				par[a] = v;
				if (!vist[a] && workCycle(a, gph, vist, par))
					return 1;
				else if (vist[a] == 1)
				{
					//check all the nodes that are being checked
					printCycle(par, v, a);
					return 1;
				}
			}
		}
	}
	vist[v] = 2;
	return 0;
}

int main(int argc, char* argv[])
{
	if(argc < 2)
	{
		printf(" manager error.\n");
		exit(1);
	}
	n = rdSequenceFile();
	signal(SIGCHLD, DeadlockCnt);
	srand(time(NULL));
	semaphoreid1 = semget(SEMAPHOREKEY, 6, IPC_CREAT | 0666);
	if (semaphoreid1 < 0)
	{
		perror("semget");
		exit(EXIT_FAILURE);
	}
	int a;
	for (a = 0; a <= 5; a++)
	{
		if (semctl(semaphoreid1, a, SETVAL, 1) < 0)
		{
			perror("semctl");
			exit(EXIT_FAILURE);
		}
	}
	init(); //initialize matrix here
	float p = 0.2; //p probability
	p = atof(argv[1]);
	int ind = 0;
	while (ind < n)
	{
		float randNum = rand() * 1.0 / RAND_MAX;
		//taking random values
		if (randNum < p) //checking the value of p
		{
			findDeadlock();
		}
		else
		{
			int prid = fork();
			if (prid == 0)
			{
				char dir[2];
				dir[0] = seq[ind];
				dir[1] = '\0';
				char sind[10];
				sprintf(sind, "%d", ind);
				char snum[10];
				sprintf(snum, "%d", n);
				//execlp() system call
				execlp( "./train", "./train", dir, sind, snum, (char *)(NULL));
				exit(0);
			}
			else
			{
				processIdArray[ind] = prid;
				ind++;
			}
		}
	}
	while (zerochild < n)
	{
		sleep(1);
		findDeadlock();
	}
	printf("No deadlock .\n");
	return 0;
}

