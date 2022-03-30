#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/wait.h>


/*Buffers structures - critical sections */

/*FIFO buffer*/
struct Buffer_1{
	int head, tail, size;
	unsigned capacity;
	char* content;
};

/*LIFO buffer*/
struct Buffer_2{
	int head, size;
	unsigned capacity;
	char* content;

};

/*FIFO buffer*/
struct Buffer_3{
	int head, tail, size;
	unsigned capacity;
	char* content;
};

size_t B1_MAX = 7;
size_t B2_MAX = 6;
size_t B3_MAX = 5;

char a = 'a';
char b = 'b';
char c = 'c';
char d = 'd';
char e = 'e';
char f = 'f';
char g = 'g';
char h = 'h';

sem_t * buf1_full;
sem_t * buf1_empty;
pthread_mutex_t * buf1_mutex;

sem_t * buf2_full;
sem_t * buf2_empty;
pthread_mutex_t * buf2_mutex;

sem_t * buf3_full;
sem_t * buf3_empty;
pthread_mutex_t * buf3_mutex;

pthread_mutexattr_t mutex_attr;

void *create_shared_memory(size_t size)
{
	int protection = PROT_READ | PROT_WRITE;
	int visibility = MAP_SHARED | MAP_ANONYMOUS;
	return mmap(NULL, size, protection, visibility, -1, 0);
}




/* Adds new elements for buffer_1*/
void Producer(struct Buffer_1* buffer_1, char item){
   
    sem_wait(buf1_full);   
    pthread_mutex_lock(buf1_mutex);     	


    buffer_1->tail = (buffer_1->tail + 1)
                  % buffer_1->capacity;    
    buffer_1->content[buffer_1->tail] = item;
    buffer_1->size = buffer_1->size + 1;
    
	printf("Added to buffer_1 : %c\n", item); 
    


    pthread_mutex_unlock(buf1_mutex);
    sem_post(buf1_empty);	
}
 




/* Takes elements from buffer_1 and adds them to buffer_2 */
void Transporter_2(struct Buffer_1* buffer_1, struct Buffer_2* buffer_2)
{

    sem_wait(buf1_empty);
    pthread_mutex_lock(buf1_mutex);
    
    char item = buffer_1->content[buffer_1->head];
    buffer_1->head = (buffer_1->head + 1)
                   % buffer_1->capacity;
    buffer_1->size = buffer_1->size - 1;

    pthread_mutex_unlock(buf1_mutex);
    sem_post(buf1_full);
    

    sem_wait(buf2_full);
    pthread_mutex_lock(buf2_mutex);
    
    buffer_2->head = (buffer_2->head + 1)
                  % buffer_2->capacity;
    buffer_2->content[buffer_2->head] = item;
    buffer_2->size = buffer_2->head+1;
    printf("Added to buffer_2 : %c\n", item); 
	
	
	

    pthread_mutex_unlock(buf2_mutex);
    sem_post(buf2_empty);

}


/* Takes elements from buffer_2 and adds them to buffer_3 */
void Transporter_3(struct Buffer_2* buffer_2, struct Buffer_3* buffer_3)
{
    


    sem_wait(buf2_empty);
    pthread_mutex_lock(buf2_mutex); 
	
    char item = buffer_2->content[buffer_2->head]; 
    buffer_2->head = (buffer_2->head - 1) /* LIFO */
                   % buffer_2->capacity;
    buffer_2->size = buffer_2->head+1;
	
    pthread_mutex_unlock(buf2_mutex);	
    sem_post(buf2_full);



    sem_wait(buf3_full);
    pthread_mutex_lock(buf3_mutex);
    
    buffer_3->tail = (buffer_3->tail + 1)
                  % buffer_3->capacity;
    buffer_3->content[buffer_3->tail] = item;
    buffer_3->size = buffer_3->size + 1;
    	 printf("Added to buffer_3 : %c\n", item); 
    pthread_mutex_unlock(buf3_mutex);
    sem_post(buf3_empty);

}


/* Consumes elements from buffer_3 */
void Consumer(struct Buffer_3* buffer_3)
{

    sem_wait(buf3_empty);
    pthread_mutex_lock(buf3_mutex);    

    char item = buffer_3->content[buffer_3->head];
	 printf(" Item consumed from buffer 3 : %c \n", item); 
    buffer_3->head = (buffer_3->head + 1)
                   % buffer_3->capacity;
    buffer_3->size = buffer_3->size - 1;

    pthread_mutex_unlock(buf3_mutex);
    sem_post(buf3_full);

}


int main()
{
	int proc_id;
	size_t shmem_size;
	void *shmem;
	
	shmem_size =  6*sizeof(sem_t)+ 3*sizeof(pthread_mutex_t) + 
	sizeof(struct Buffer_1) + sizeof(struct Buffer_2) + 
		sizeof(struct Buffer_3) + sizeof(char)*(B1_MAX + B2_MAX + B3_MAX); 
	shmem = create_shared_memory(shmem_size);
	
	buf1_empty = (sem_t*)shmem;
	buf1_full = buf1_empty + 1;
	buf2_empty = buf1_full + 1;
	buf2_full = buf2_empty + 1;
	buf3_empty = buf2_full + 1;
	buf3_full = buf3_empty + 1;
	
	buf1_mutex = (pthread_mutex_t*)(buf3_full +1);
	buf2_mutex = buf1_mutex + 1;
	buf3_mutex = buf2_mutex + 1;

	struct Buffer_1* buffer_1 = (struct Buffer_1*)(buf3_mutex +1);
	(buffer_1->capacity) = B1_MAX;
    	buffer_1->head = buffer_1->size = 0;
	buffer_1->tail = B1_MAX - 1;
	buffer_1 -> content = (char*)(buffer_1 + sizeof(struct Buffer_1));
	
	struct Buffer_2* buffer_2 = (struct Buffer_2*)(buffer_1->content + 
		B1_MAX*sizeof(char));
	buffer_2->capacity = B2_MAX;
    	buffer_2->head = -1;
	buffer_2 -> size = (buffer_2->head +1);
	buffer_2 -> content = (char*)(buffer_2 + sizeof(struct Buffer_2));

	struct Buffer_3* buffer_3 = (struct Buffer_3*)(buffer_2->content + 
		B2_MAX*sizeof(char));
	buffer_3->capacity = B3_MAX;
    	buffer_3->head = buffer_3->size = 0;
	buffer_3->tail = B3_MAX - 1;
	buffer_3 -> content = (char*)(buffer_3 + sizeof(struct Buffer_3));
	
	pthread_mutexattr_init(&mutex_attr);
	pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED);	
	
    sem_init(buf1_empty, 1, 0);
    sem_init(buf1_full, 1, B1_MAX );
	
    sem_init(buf2_empty, 1, 0);
    sem_init(buf2_full, 1, B2_MAX );
	
    sem_init(buf3_empty, 1, 0);
    sem_init(buf3_full, 1, B3_MAX );

	pthread_mutex_init(buf1_mutex, &mutex_attr);
	pthread_mutex_init(buf2_mutex, &mutex_attr);
	pthread_mutex_init(buf3_mutex, &mutex_attr);
	
	

	
	int n = 8;
	
	for (int i = 0; i < 4 ; i++)
	{
		proc_id = fork();
		if (proc_id == 0)
		{	
			if(i ==0 ){					
					Producer(buffer_1, a );
					Producer(buffer_1, b );
					Producer(buffer_1, c );
					Producer(buffer_1, d );
					Producer(buffer_1, e );
					Producer(buffer_1, f );
					Producer(buffer_1, g ); 
					Producer(buffer_1, h ); 

			}
			else if(i ==1){
					for(int j = 0; j<n ; ++j){
						Transporter_2(buffer_1, buffer_2);
					}
			}
			else if(i ==2){
					for(int k = 0; k<n ; ++k){
						
						Transporter_3(buffer_2, buffer_3);
					}
			}
			else if(i == 3){
					for(int l = 0; l<n ; ++l){
						
						Consumer(buffer_3);
					}
			}
			
			exit(0);
			break;
		}
	}
	sleep(10);
	
	killpg(proc_id, SIGKILL);
	sem_destroy(buf1_full);
	sem_destroy(buf2_full);
	sem_destroy(buf3_full);
	sem_destroy(buf1_empty);
	sem_destroy(buf2_empty);
	sem_destroy(buf3_empty);
	pthread_mutex_destroy(buf1_mutex);
	pthread_mutex_destroy(buf2_mutex);
	pthread_mutex_destroy(buf3_mutex);
	pthread_mutexattr_destroy(&mutex_attr);
	munmap(shmem, shmem_size);
	return 0;
}

