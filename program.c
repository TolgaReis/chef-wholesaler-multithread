#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sem.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <errno.h>

/* Macro Constants */
#define TRUE 1
#define FALSE 0
#define THREAD_NUM 6
#define SEM_NUM 4
#define READ_BLK_SIZE 3
/* Macro Constants End*/

/* Function Definitions */
int select_chef(char*);
void print_take_msg(const char, const int);
/* Function Definitions End*/

/* Thread Definitons */
void *chef(void*);
/* Thread Definitions End*/

/* Globals */
sem_t mutex;
int semaphore_set; 
char *common_bag;
char *lack[6] = {"flour and walnut",
                 "flour and milk", 
                 "milk and sugar",  
                 "flour and sugar", 
                 "sugar and walnut",
                 "milk and walnut"};
int condition;
int** chef_num;
/* Globals End */

int main(int argc, char* argv[])
{
    char *S_KEY = "s-key";
	condition = 1; 
	if(argc != 3)
	{
		fprintf(stderr, "Wrong usage! Use: ./program -i filePath\n");
		exit(EXIT_FAILURE);
	}

    int option;
    char* file_path;
	while ((option = getopt (argc, argv, "i")) != -1)
  	{
	    if(option == 'i')
	    {
        	file_path = argv[optind];
        }
	    else
      	{
	      	fprintf(stderr, "Wrong usage! Use: ./program -i filePath\n");
			exit(EXIT_FAILURE);
	    }
	}

	int fd;
    int fd_key;
	if(((fd = open(file_path, O_RDONLY)) == -1) || 
       ((fd_key = open(S_KEY, O_CREAT)) == -1))
	{
		perror("open");
		exit(EXIT_FAILURE);
	}

    key_t sem_key;
	if ((sem_key = ftok(S_KEY, 'a')) == -1) 
    { 
        perror("ftok"); 
        exit(EXIT_FAILURE);
    }
    if ((semaphore_set = semget(sem_key, SEM_NUM, IPC_CREAT | 0660)) == -1) 
    {
        perror("semget");
        exit(EXIT_FAILURE);
    }
       
    for(int i = 0; i < SEM_NUM; i++)
    {
    	if (semctl(semaphore_set, i, SETVAL, 0) == -1) 
        {
            perror("semctl");
            exit(EXIT_FAILURE);
    	}
    }

    chef_num = (int**)malloc(sizeof(int*)*THREAD_NUM);
    pthread_t tid[THREAD_NUM];
	for(int i = 0; i < THREAD_NUM; i++)
	{
        chef_num[i] = (int*)malloc(sizeof(int));
		*chef_num[i] = i + 1;
        pthread_create( &tid[i], NULL, chef, chef_num[i]);
	}

    srand(time(NULL));
    sem_init(&mutex, 0, 0);
    common_bag = (char*)malloc(sizeof(char)*2);
    
    int read_bytes;
    char *read_buffer = (char*)malloc(sizeof(char)*3);
    struct sembuf semaphore_buf[SEM_NUM];
	semaphore_buf[0].sem_flg = 0;
	semaphore_buf[1].sem_flg = 0;
	semaphore_buf[2].sem_flg = 0;
	semaphore_buf[3].sem_flg = 0;
	while(TRUE)
	{
		if(((read_bytes = read(fd, read_buffer, 3)) == -1) && (errno == EINTR))
        {
            perror("read");
            exit(EXIT_FAILURE);
        }
		if(read_bytes <= 0)
		{
			condition = 0;
			break;
		}

        int selection;
        if((selection = select_chef(read_buffer)) == -1)
        {
            fprintf(stderr, "Invalid character in file!\n");
            exit(EXIT_FAILURE);
        }
        else
        {
            common_bag[0] = read_buffer[0];
		    common_bag[1] = read_buffer[1];
        }
        

        semaphore_buf[0].sem_op = 0;
		semaphore_buf[1].sem_op = 0;
        semaphore_buf[2].sem_op = 1;
		semaphore_buf[3].sem_op = 1;
        switch (selection)
        {
        case 1:
			semaphore_buf[0].sem_num = 0;
			semaphore_buf[1].sem_num = 1;
			semaphore_buf[2].sem_num = 0;
			semaphore_buf[3].sem_num = 1;
            break;
        case 2:
			semaphore_buf[0].sem_num = 0;
			semaphore_buf[1].sem_num = 2;
			semaphore_buf[2].sem_num = 0;
			semaphore_buf[3].sem_num = 2;
            break;
        case 3:
			semaphore_buf[0].sem_num = 0;
			semaphore_buf[1].sem_num = 3;
			semaphore_buf[2].sem_num = 0;
			semaphore_buf[3].sem_num = 3;
            break;
        case 4:
			semaphore_buf[0].sem_num = 1;
			semaphore_buf[1].sem_num = 2;
			semaphore_buf[2].sem_num = 1;
			semaphore_buf[3].sem_num = 2;    
            break;
        case 5:
			semaphore_buf[0].sem_num = 1;
			semaphore_buf[1].sem_num = 3;
			semaphore_buf[2].sem_num = 1;
			semaphore_buf[3].sem_num = 3;            
            break;
        case 6:
			semaphore_buf[0].sem_num = 2;
			semaphore_buf[1].sem_num = 3;
			semaphore_buf[2].sem_num = 2;
			semaphore_buf[3].sem_num = 3;            
            break;
        
        default:
            break;
        }
        printf("the wholesaler delivers %s\n", lack[selection - 1]);
        if (semop(semaphore_set, semaphore_buf, SEM_NUM) == -1)
		{
			perror("semop"); 
            exit(EXIT_FAILURE);
		}
		printf("the wholesaler is waiting for the dessert\n");
		sem_wait(&mutex);
		printf("the wholesaler has obtained the dessert and left to sell it\n");
	}

    semaphore_buf[0].sem_num = 0;
	semaphore_buf[0].sem_op = 3;
    semaphore_buf[1].sem_num = 1;
	semaphore_buf[1].sem_op = 3;
    semaphore_buf[2].sem_num = 2;
	semaphore_buf[2].sem_op = 3;
	semaphore_buf[3].sem_num = 3;
	semaphore_buf[3].sem_op = 3;
	if (semop(semaphore_set, semaphore_buf, SEM_NUM) == -1)
	{
		perror("semop"); 
        exit(EXIT_FAILURE);
    }

	for(int i = 0; i < THREAD_NUM; i++)
	{
		pthread_join(tid[i], NULL); 
	}

	if((close(fd) == -1) || ((close(fd_key) == -1)))
	{
		perror("close");
		exit(EXIT_FAILURE);
	}
    for(int i = 0; i < THREAD_NUM; i++)
        free(chef_num[i]);
    free(chef_num);
    free(common_bag);
    free(read_buffer);
	unlink(S_KEY);
	return 0;
}
int select_chef(char* read_buffer)
{
    if((read_buffer[0] == 'F' && read_buffer[1] == 'W') || 
       (read_buffer[0] == 'W' && read_buffer[1] == 'F'))
	{
		return 1;
	}
	else if((read_buffer[0] == 'F' && read_buffer[1] == 'M') || 
            (read_buffer[0] == 'M' && read_buffer[1] == 'F'))
	{
		return 2;
	}
	else if((read_buffer[0] == 'M' && read_buffer[1] == 'S') || 
            (read_buffer[0] == 'S' && read_buffer[1] == 'M'))
	{
		return 3;
	}
	else if((read_buffer[0] == 'S' && read_buffer[1] == 'F') || 
            (read_buffer[0] == 'F' && read_buffer[1] == 'S'))
	{
		return 4;
	}
	else if((read_buffer[0] == 'W' && read_buffer[1] == 'S') || 
            (read_buffer[0] == 'S' && read_buffer[1] == 'W'))
	{
		return 5;
	}	
	else if((read_buffer[0] == 'W' && read_buffer[1] == 'M') || 
            (read_buffer[0] == 'M' && read_buffer[1] == 'W'))
	{
		return 6;
	}
    else 
    {
        return -1;
    }
}

void *chef(void *arg)
{
    int chef_num = *(int*)(arg);

	struct sembuf semaphore_buf[SEM_NUM];
	semaphore_buf[0].sem_flg = 0;
	semaphore_buf[1].sem_flg = 0;
	while(TRUE)
	{
        printf("chef%d is waiting for %s\n", chef_num, lack[chef_num-1]);
        semaphore_buf[0].sem_op = -1;
		semaphore_buf[1].sem_op = -1;
        switch(chef_num)
        {
        case 1:
            semaphore_buf[0].sem_num = 0;
			semaphore_buf[1].sem_num = 1;
            break;
        case 2:
            semaphore_buf[0].sem_num = 0;
			semaphore_buf[1].sem_num = 2;
            break;
        case 3:
            semaphore_buf[0].sem_num = 0;
			semaphore_buf[1].sem_num = 3;
            break;
        case 4:
            semaphore_buf[0].sem_num = 1;
			semaphore_buf[1].sem_num = 2;
            break;
        case 5:
            semaphore_buf[0].sem_num = 1;
			semaphore_buf[1].sem_num = 3;
            break;
        case 6:
            semaphore_buf[0].sem_num = 2;
			semaphore_buf[1].sem_num = 3;
            break;
        default:
            break;
        }
		if(semop(semaphore_set, semaphore_buf, 2) == -1)
		{
			perror("semop"); 
            exit(EXIT_FAILURE);
	    }

	    if(!condition)
	    	break;

        print_take_msg(common_bag[0], chef_num);
        print_take_msg(common_bag[1], chef_num);
		printf("chef%d is preparing the dessert\n", chef_num);
		sleep((rand() % 5) + 1);
		printf("chef%d has delivered the dessert to the wholesaler\n", chef_num);
		sem_post(&mutex);
	}
	pthread_exit(NULL);
}
void print_take_msg(const char type, const int chef_num)
{
    switch(type)
    {
        case 'M':
            printf("chef%d has taken the milk\n", chef_num);
            break;
        case 'F':
            printf("chef%d has taken the flour\n", chef_num);
            break;
        case 'W':
            printf("chef%d has taken the walnut\n", chef_num);
            break;
        case 'S':
            printf("chef%d has taken the sugar\n", chef_num);
            break;
        default:
            break;
    }
}