#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <thread>
#include <sys/mman.h>
#include <sys/wait.h>
#include <iostream>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include "monitor.h"
using namespace std;



/*Buffers structures - critical sections */

struct Buffer_1{
	int head, tail, size;
	unsigned capacity;
	char* content;
};

struct Buffer_2{
	int head, size;
	unsigned capacity;
	char* content;

};

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


Monitor buf1_monitor;
Monitor buf2_monitor;
Monitor buf3_monitor;

Condition buf1_empty;
Condition buf2_empty;
Condition buf3_empty;

Condition buf1_full;
Condition buf2_full;
Condition buf3_full;


int buf1_count;
int buf2_count;
int buf3_count;

struct Buffer_1* buffer_1;
struct Buffer_2* buffer_2;
struct Buffer_3* buffer_3;




/* Adds new elements for buffer_1*/
void Producer(struct Buffer_1* buffer_1, char item){
   
    	
	buf1_monitor.enter();
	if(buf1_count == B1_MAX){
			buf1_monitor.wait(buf1_empty);
	}
	
    buffer_1->tail = (buffer_1->tail + 1)
                  % buffer_1->capacity;    
    buffer_1->content[buffer_1->tail] = item;
    buffer_1->size = buffer_1->size + 1;
    
	cout<<"Added to buffer_1 : "<< item << endl; 
    
	buf1_count++;
	buf1_monitor.signal(buf1_full);
	buf1_monitor.leave();	
}
 




/* Takes elements from buffer_1 and adds them to buffer_2 */
void Transporter_2(struct Buffer_1* buffer_1, struct Buffer_2* buffer_2,int n)
{
for(int j = 0; j<n ; ++j){
	
    buf1_monitor.enter();
	if(buf1_count == 0){
		
			buf1_monitor.wait(buf1_full);
			
	}
    
    char item = buffer_1->content[buffer_1->head];
    buffer_1->head = (buffer_1->head + 1)
                   % buffer_1->capacity;
    buffer_1->size = buffer_1->size - 1;

	buf1_count--;
    buf1_monitor.signal(buf1_empty);
	buf1_monitor.leave();
    

    buf2_monitor.enter();
	if(buf2_count == B2_MAX){
			buf2_monitor.wait(buf2_empty);
	}
    
    buffer_2->head = (buffer_2->head + 1)
                  % buffer_2->capacity;
    buffer_2->content[buffer_2->head] = item;
    buffer_2->size = buffer_2->head+1;
    cout<<"Added to buffer_2 : "<< item << endl;  
	
	
	

    buf2_count++;
	buf2_monitor.signal(buf2_full);
	buf2_monitor.leave();
}
}


/* Takes elements from buffer_2 and adds them to buffer_3 */
void Transporter_3(struct Buffer_2* buffer_2, struct Buffer_3* buffer_3,int n)
{
    
for(int j = 0; j<n ; ++j){

    buf2_monitor.enter();
	if(buf2_count == 0){
			buf2_monitor.wait(buf2_full);
	}
	
    char item = buffer_2->content[buffer_2->head]; 
    buffer_2->head = (buffer_2->head - 1) /* LIFO */
                   % buffer_2->capacity;
    buffer_2->size = buffer_2->head+1;
	
    buf2_count--;
    buf2_monitor.signal(buf2_empty);
	buf2_monitor.leave();



    buf3_monitor.enter();
	if(buf3_count == B3_MAX){
			buf3_monitor.wait(buf3_empty);
	}
    
    buffer_3->tail = (buffer_3->tail + 1)
                  % buffer_3->capacity;
    buffer_3->content[buffer_3->tail] = item;
    buffer_3->size = buffer_3->size + 1;
    	 cout<<"Added to buffer_3 : "<< item << endl;  
    
	buf3_count++;
	buf3_monitor.signal(buf3_full);
	buf3_monitor.leave();

}
}


/* Consumes elements from buffer_3 */
void Consumer(struct Buffer_3* buffer_3,int n)
{
for(int j = 0; j<n ; ++j){
	
    buf3_monitor.enter();
	if(buf3_count == 0){
			buf3_monitor.wait(buf3_full);
	}    

    char item = buffer_3->content[buffer_3->head];
	cout<<" Item consumed from buffer 3 : "<< item << endl;  
	 
    buffer_3->head = (buffer_3->head + 1)
                   % buffer_3->capacity;
    buffer_3->size = buffer_3->size - 1;

    buf3_count--;
    buf3_monitor.signal(buf3_empty);
	buf3_monitor.leave();

}
}

int main()
{
	buf1_count = 0;
	buf2_count = 0;
	buf3_count = 0;

	buffer_1 = new Buffer_1();

	(buffer_1->capacity) = B1_MAX;

    	buffer_1->head = buffer_1->size = 0;

	buffer_1->tail = B1_MAX - 1;

	buffer_1->content = new char[buffer_1->capacity];


	buffer_2 = new Buffer_2();
	buffer_2->capacity = B2_MAX;
    	buffer_2->head = -1;
	buffer_2 -> size = (buffer_2->head +1);
	buffer_2->content = new char[buffer_2->capacity];

	buffer_3 = new Buffer_3();
	buffer_3->capacity = B3_MAX;
    	buffer_3->head = buffer_3->size = 0;
	buffer_3->tail = B3_MAX - 1;
	buffer_3->content = new char[buffer_3->capacity];

	
	int n = 7;
	
				
	thread p1(Producer, buffer_1, a );
	sleep(0.1);
	thread p2(Producer, buffer_1, b );
	sleep(0.1);
	thread p3(Producer, buffer_1, c );
	sleep(0.1);
	thread p4(Producer, buffer_1, d );
	sleep(0.1);
	thread p5(Producer, buffer_1, e );
	sleep(0.1);
	thread p6(Producer, buffer_1, f );
	sleep(0.1);
	thread p7(Producer, buffer_1, g ); 
	//sleep(0.1);
	//thread p8(Producer, buffer_1, h ); 
	sleep(2);
	thread t2(Transporter_2, buffer_1, buffer_2,n);
	sleep(2);
	thread t3(Transporter_3,buffer_2, buffer_3,n);
	sleep(2);
	thread c1(Consumer,buffer_3,n);


	p1.join();
	p2.join();
	p3.join();
	p4.join();
	p5.join();
	p6.join();
	p7.join();
	//p8.join();
	t2.join();
	t3.join();
	c1.join();	
	
	


	sleep(4);
	return 0;
}

