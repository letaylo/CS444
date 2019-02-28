/************************
CS444/544 Operating System

Week 07: Solving the producer/consumer problem

Locking scheme description:
        
I have one lock, which is locked initially. Then a while check is made for both producer and consumer on if the queue is full or empty, respectively. If true, the lock is released
and the thread sleeps for a bit before reacquiring the lock. When the thread finally gets out of the check, the thread either produces or consumes and releases the lock.

        
Name: Lee Taylor

Clarkson University  SP2019
 **************************/ 

/***** Includes *****/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifdef UNIX
	#include <pthread.h>
	#include <unistd.h>
#endif

#ifdef WINDOWS
	#include <windows.h>
#endif


/***** Defines *****/
#define PRODUCER_SLEEP_S	1		
#define CONSUMER_SLEEP_S	1
#define QUEUESIZE 			5
#define LOOP 				10


/***** Function prototypes *****/
void *producer (void *args);
void *consumer (void *args);

/***** Queue struct *****/
typedef struct {
	int buf[QUEUESIZE];
	long head, tail;
	int full, empty;  

	/*Declare the locks here */
	HANDLE mutex;
	
	

} queue;

/***** Queue function prototypes  *****/
queue *queueInit (void);
void queueDelete (queue *q);
void queueAdd (queue *q, int in);
void queueDel (queue *q, int *out);


/***** main *****/
int main ()
{
	queue *fifo;
	
	int i;

	
	unsigned int iseed = (unsigned int)time(NULL);
	srand(iseed);
	
    /******one producer and multiple consumer********/
	#ifdef UNIX    
	pthread_t pro, con[LOOP];
	#endif
	
	#ifdef WINDOWS
	HANDLE pro, con[LOOP];
	#endif
	
        

	fifo = queueInit ();
	if (fifo ==  NULL) {
		fprintf (stderr, "main: Queue Init failed.\n");
		exit (1);
	}
	
	#ifdef UNIX
		pthread_create (&pro, NULL, producer, fifo);
		for(i=0; i<LOOP; i++)
		{
			pthread_create (&con[i], NULL, consumer, fifo);
		}
	#endif
	
	#ifdef WINDOWS
		pro = CreateThread(NULL, 0,(LPTHREAD_START_ROUTINE) producer, fifo, 0, NULL);
		for(i=0; i<LOOP; i++)
		{
	#ifdef WINDOWS
			/* Simulate Consumers' random arrvial */
			Sleep((rand()%2)*1000);
	#endif
		    con[i] = CreateThread(NULL, 0,(LPTHREAD_START_ROUTINE) consumer, fifo, 0, NULL);		
		}
	#endif
	
	
	#ifdef UNIX
		pthread_join (pro, NULL);
		for(i=0; i<LOOP; i++)
		{
			pthread_join (con[i], NULL);	
		}
	#endif
	
	#ifdef WINDOWS
		WaitForSingleObject(pro, INFINITE);
        /*******Wait for all the threads complete**********/      
		WaitForMultipleObjects(LOOP, con, TRUE, INFINITE);	

	#endif
	
	queueDelete (fifo);

    #ifdef WINDOWS
	system("pause");
	#endif
	
	return 0;
}

/***** producer *****/
void *producer (void *q)
{

	queue *fifo;
	int i;

	fifo = (queue *)q;


    /**
      obtain the lock and release the lock somewhere
     **/

	for (i = 0; i < LOOP; i++) 
	{

        /*******Obtain the locks somewhere here **********/
		

		#ifdef UNIX
		while (fifo->full) 
		{
		}
		#endif
		
        #ifdef WINDOWS
		WaitForSingleObject(fifo->mutex, INFINITE);
        #endif
		
		
		while (fifo->full == 1) 
		{
			
			/* sleep */
			#ifdef UNIX
				usleep ( PRODUCER_SLEEP_S * 1000000); 
			#endif
		
			#ifdef WINDOWS
				ReleaseMutex(fifo->mutex);
				Sleep ( PRODUCER_SLEEP_S * 1000);	
				WaitForSingleObject(fifo->mutex, INFINITE);
			#endif
		}
		
		
		queueAdd (fifo, i+1);
		printf ("producer: produced %d th.\n",i+1);
		
		/* sleep */
		#ifdef UNIX
			usleep ( PRODUCER_SLEEP_S * 1000000); 
		#endif
		
		#ifdef WINDOWS
			Sleep ( PRODUCER_SLEEP_S * 1000);	
		#endif

        /*******Release the locks**********/
        #ifdef UNIX

        #endif

        
        #ifdef WINDOWS
		ReleaseMutex(fifo->mutex);
        #endif
	}

	return (NULL);
}

/***** consumer *****/
void *consumer (void *q)
{
	queue *fifo;
	int d;

	fifo = (queue *)q;

	/* Simulate Consumers' random arrvial */
	#ifdef UNIX
		usleep ( (rand()%LOOP) * 1000000); 
	#endif
		

    /**
      obtain the lock and release the lock somewhere
     **/

    #ifdef UNIX
	while (fifo->empty) 
	{
	}
	#endif
	
	
    #ifdef WINDOWS
	WaitForSingleObject(fifo->mutex, INFINITE);
    #endif
	
	while (fifo->empty == 1) 
	{
		#ifdef UNIX
			usleep ( CONSUMER_SLEEP_S * 1000000); 
			while (fifo->empty) 
			{
			}
		#endif
	
		#ifdef WINDOWS
			ReleaseMutex(fifo->mutex);
			Sleep ( (rand()%LOOP) * 1000);	
			WaitForSingleObject(fifo->mutex, INFINITE);
		#endif
	}
	
	/* sleep */
	#ifdef UNIX
		usleep ( CONSUMER_SLEEP_S * 1000000); 
	#endif

	#ifdef WINDOWS
		Sleep ( CONSUMER_SLEEP_S * 1000);	
	#endif

	queueDel (fifo, &d);
	printf ("------------------------------------>consumer: recieved %d.\n", d);
		
    #ifdef UNIX

    #endif


    #ifdef WINDOWS
	ReleaseMutex(fifo->mutex);
    #endif
	

	return (NULL);
}


/***** queueInit *****/
queue *queueInit (void)
{
	queue *q;
	int i;
	
	q = (queue *)malloc (sizeof (queue));
	if (q == NULL) return (NULL);

	for(i=0;i<QUEUESIZE;i++)
	{
		q->buf[i]=0;
	}
	q->empty = 1;
	q->full = 0;
	q->head = 0;
	q->tail = 0;

    /*Initialize the locks here	*/
	q->mutex = CreateMutex(NULL, FALSE, NULL);



	return (q);
}


/***** queueDelete *****/
void queueDelete (queue *q)
{

	/* free the locks here*/
	CloseHandle(q->mutex);
	
	/* free memory used for queue */
	free (q);
}

/***** queueAdd *****/
void queueAdd (queue *q, int in)
{
	q->buf[q->tail] = in;
	q->tail++;
	if (q->tail == QUEUESIZE)
		q->tail = 0;
	if (q->tail == q->head)
		q->full = 1;
	q->empty = 0;

	return;
}

/***** queueDel *****/
void queueDel (queue *q, int *out)
{
	*out = q->buf[q->head];
	q->buf[q->head]=0;

	q->head++;

	if (q->head == QUEUESIZE)
		q->head = 0;
	if (q->head == q->tail)
		q->empty = 1;
	q->full = 0;

	return;
}
