#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <time.h>
#include <sys/sem.h>

#define SEQLENGTH 2000
#define SEMAPHOREKEY 2058 //semaphore key

#define MUTX_1_1 5
#define MUTX_2 4
#define NORTH 0
#define WEST 1
#define SOUTH 2
#define EAST 3

//request, release and acquire commands
#define REQUEST 11
#define RELEASE 10
#define ACQUIRE 12

#define LINEBYTES (4*sizeof(int))

char *dishaName[] = {"North","West","South","East","Junction"};
int semaphoreid1; //semaphore set identifier
int ind;
int n; //number of train1s from the sequence
FILE *flName;

//create method to wait on a semaphore
int sem_wait(int semaphoreid1, int subsem)
{
	struct sembuf sop;
	sop.sem_num = subsem;
	sop.sem_op = -1;
	sop.sem_flg = 0;
	return semop(semaphoreid1, &sop, 1);
}

int sem_signal(int semaphoreid1, int subsem)
{
	struct sembuf sop;
	sop.sem_num = subsem;
	sop.sem_op = 1;
	sop.sem_flg = 0;
	return semop(semaphoreid1, &sop, 1);
}

/*
Create switch statement to get the dirxc of the train1
*/
int getdirxc(char c)
{
	int dirx;
	switch (c)
	{
	case 'N' : dirx = NORTH;
		break;
	case 'W' : dirx = WEST;
		break;
	case 'E' : dirx = EAST;
		break;
	case 'S' : dirx = SOUTH;
		break;
	}
	return dirx;
}

//now, update the matrix
void update(int type, int direction)
{

	int i,j;
	flName = fopen("matrix.txt","r");
	if(flName == NULL)
	{
		printf("Error reading file\n");
		exit(1);
	}
	int matrix[n][4];
	for(i=0;i<n;i++)
	{
		for(j=0;j<4;j++)
		{
			fscanf(flName,"%d",&matrix[i][j]);
		}
	}
	fclose(flName);
	matrix[ind][direction] = type - 10;
	flName = fopen("matrix.txt","w");
	if(flName == NULL)
	{
		printf("Error opening file\n");
		exit(1);
	}
	for(i=0;i<n;i++)
	{
		for(j=0;j<4;j++)
		{ 
			fprintf(flName,"%d ",matrix[i][j]);
		}
		fprintf(flName, "\n");
	}
	fclose(flName);
}

int main(int argc, char *argv[])
{
	if (argc < 4)
	{
		printf("Usage: ./train direction index");
		exit(EXIT_FAILURE);
	}
	char cdirx = argv[1][0];
	ind = atoi(argv[2]);
	n = atoi(argv[3]);
	int dirx = getdirxc(cdirx);
	int pid = getpid();
	printf("Train %d: %s train started\n",pid,dishaName[dirx]);
	semaphoreid1 = semget(SEMAPHOREKEY, 6, 0666);
	int right = (dirx + 1) % 4;
	sem_wait(semaphoreid1, MUTX_1_1);
	update(REQUEST, dirx);
	sem_signal(semaphoreid1, MUTX_1_1);

	printf("Train %d: Requests %s lock\n",pid,dishaName[dirx]);
	sem_wait(semaphoreid1, dirx);
	printf("Train %d: Acquires %s lock\n",pid,dishaName[dirx]);


	sem_wait(semaphoreid1, MUTX_1_1);
	update(ACQUIRE, dirx);
	update(REQUEST, right);
	sem_signal(semaphoreid1, MUTX_1_1);

	printf("Train %d: Requests %s lock\n",pid,dishaName[right]);
	sem_wait(semaphoreid1, right);
	printf("Train %d: Acquires %s lock\n",pid,dishaName[right]);

	sem_wait(semaphoreid1, MUTX_1_1);
	update(ACQUIRE, right);
	sem_signal(semaphoreid1, MUTX_1_1);

	printf("Train %d: Requests %s lock\n",pid,dishaName[MUTX_2]);
	sem_wait(semaphoreid1, MUTX_2);
	printf("Train %d: Acquires %s lock; passing Junction\n",pid,dishaName[MUTX_2]);

	sleep(2);

	sem_signal(semaphoreid1, MUTX_2);
	printf("Train %d: Releases %s lock\n",pid,dishaName[MUTX_2]);

	sem_signal(semaphoreid1, dirx);
	printf("Train %d: Releases %s lock\n",pid,dishaName[dirx]);
	sem_wait(semaphoreid1, MUTX_1_1);
	update(RELEASE, dirx);
	sem_signal(semaphoreid1, MUTX_1_1);

	sem_signal(semaphoreid1, right);
	printf("Train %d: Releases %s lock\n",pid,dishaName[right]);
	sem_wait(semaphoreid1, MUTX_1_1);
	update(RELEASE, right);
	sem_signal(semaphoreid1, MUTX_1_1);
	return 0;
}

