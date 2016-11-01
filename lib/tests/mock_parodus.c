/**
 * @file mock_parodus.c
 *
 * @description This file is used to mock the parodus service, for use with unit tests
 *
 * Copyright (c) 2015  Comcast
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
//#include <cJSON.h>
#include <sys/time.h>
#include <sys/sysinfo.h>
#include <pthread.h>

#include <getopt.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <wrp-c/wrp-c.h>
#include <nanomsg/nn.h>
#include <nanomsg/pipeline.h>


/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/


#define PARODUS_UPSTREAM "tcp://127.0.0.1:6666"

#define IOT "iot"
#define GET_SET "get_set"

#define TEST_MSG_BUF_LEN 6000

#define END_PIPE_NAME "mock_parodus_end.txt"
#define END_PIPE_MSG  "--END--\n"
#define PIPE_BUFLEN 32
#define NAME_BUFLEN 128

/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/

typedef struct
{
    char test_msgs_file[NAME_BUFLEN];
    unsigned long test_msg_delay;
    unsigned long test_msg_count;
		unsigned long create_pipe_opt;
} Cfg_t;


typedef struct ParodusMsg__
{
	//noPollMsg * msg;
	void * payload;
	size_t len;
	struct ParodusMsg__ *next;
} ParodusMsg;

typedef struct UpStreamMsg__
{
	void *msg;
	size_t len;
	struct UpStreamMsg__ *next;
} UpStreamMsg;

typedef struct reg_client__
{
	int sock;
	char service_name[32];
	char url[100];
} reg_client;


/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
static FILE *test_msgs_fp = NULL;

static Cfg_t Cfg;
static int numOfClients = 0;

//Currently set the max mumber of clients as 10
reg_client *clients[10];

static char pipe_buf[PIPE_BUFLEN];
static int end_pipe_fd = -1;
static char end_pipe_name[NAME_BUFLEN];
static char *end_pipe_msg = END_PIPE_MSG;

//static char deviceMAC[32]={'\0'}; 
static volatile bool terminated = false;
static ParodusMsg *ParodusMsgQ = NULL;
static UpStreamMsg *UpStreamMsgQ = NULL;

pthread_t UpStreamMsgThreadId;
pthread_t processUpStreamThreadId;
pthread_t messageThreadId;

pthread_mutex_t parodus_mut=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t parodus_con=PTHREAD_COND_INITIALIZER;

pthread_mutex_t nano_mut=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t nano_con=PTHREAD_COND_INITIALIZER;

pthread_mutex_t reply_mut=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t reply_con=PTHREAD_COND_INITIALIZER;
static unsigned reply_trans = 0;
static const char *trans_format = "aaaa-bbbb-####";


/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/
static int listenerOnMessage( void * msg, size_t msgSize);
void getCurrentTime(struct timespec *timer);
uint64_t getCurrentTimeInMicroSeconds(struct timespec *timer);
long timeValDiff(struct timespec *starttime, struct timespec *finishtime);

//static void loadParodusCfg(ParodusCfg * config,ParodusCfg *cfg);
static void listenerOnMessage_queue(void *msg_bytes, ssize_t msg_len);
static void initMessageHandler();
static void *messageHandlerTask();
void parStrncpy(char *destStr, const char *srcStr, size_t destSize);

static void initUpStreamTask();
static void *handle_upstream();
static void processUpStreamTask();
static void *processUpStreamHandler();
static void handleUpStreamEvents();
static void handleUpstreamMessage(wrp_msg_t *msg);

static void dbg_err (int err, const char *fmt, ...)
{
		char errbuf[100];

    va_list arg_ptr;
    va_start(arg_ptr, fmt);
    vprintf(fmt, arg_ptr);
    va_end(arg_ptr);
		printf ("%s\n", strerror_r (err, errbuf, 100));
}

static bool make_end_pipe_name (void)
{
	char *slash_pos;
	unsigned head_len, total_len;
	const char *end_pipe_name__ = END_PIPE_NAME;

	strncpy (end_pipe_name, Cfg.test_msgs_file, NAME_BUFLEN);
	end_pipe_name[NAME_BUFLEN-1] = '\0';
	slash_pos = strrchr (end_pipe_name, '/');
	if (NULL == slash_pos) {
		strncpy (end_pipe_name, end_pipe_name__, NAME_BUFLEN);
		end_pipe_name[NAME_BUFLEN-1] = '\0';
		return true;
	}
	head_len = (slash_pos + 1) - end_pipe_name;
	total_len = head_len + strlen(end_pipe_name__);
	if (total_len >= NAME_BUFLEN) {
		printf ("end pipe name too long\n");
		return false;
	}
	strcpy (slash_pos+1, end_pipe_name__);
	return true;		
}

static int open_end_pipe (bool create_pipe_opt)
{
	int err;

	if (!make_end_pipe_name ())
		return -1;

	if (create_pipe_opt) {
		err = remove (end_pipe_name);
		if ((err != 0) && (errno != ENOENT)) {
			dbg_err (errno, "Error removing pipe %s\n", end_pipe_name);
			return -1;
		}
		printf ("Removed pipe %s\n", end_pipe_name);
		err = mkfifo (end_pipe_name, 0666);
		if (err != 0) {
			dbg_err (errno, "Error creating pipe %s\n", end_pipe_name);
			return -1;
		}
		printf ("Created fifo %s\n", end_pipe_name);
	}
	end_pipe_fd = open (end_pipe_name, O_RDONLY, 0444);
	if (end_pipe_fd == -1) {
		dbg_err (errno, "Error opening pipe %s\n", end_pipe_name);
		return -1;
	}
	printf ("Opened end pipe\n");
	return 0;
}

static int read_end_pipe (void)
{
	int nbytes = read (end_pipe_fd, pipe_buf, strlen (end_pipe_msg));
	if (nbytes < 0) {
		dbg_err (errno, "Error reading pipe %s\n", end_pipe_name);
		return -1;
	}
	return nbytes;
}

void initTasks(void (* initKeypress)())

{

	initUpStreamTask();
	processUpStreamTask();
		
	initMessageHandler();
	
	if (NULL != initKeypress) 
	{
		//char testmsg[100];
  	(* initKeypress) ();

	}
	// Wait till client registers
	while (numOfClients == 0)
		sleep(1);
}

/*----------------------------------------------------------------------------*/
/*                             External Functions                             */
/*----------------------------------------------------------------------------*/

void terminateTasks()
{
	terminated = true;

	printf ("Terminating tasks\n");	
#if 0
	for (i=0; i<15; i++) 
	{
		pthread_mutex_lock (&parodus_mut);		
		if(ParodusMsgQ == NULL)
		{
		 	pthread_cond_signal(&parodus_con);
			pthread_mutex_unlock (&parodus_mut);
			break;
		}
		pthread_mutex_unlock (&parodus_mut);
	}
#endif
	pthread_join (messageThreadId, NULL);
}

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/

 /*
 * @brief To initiate UpStream message handling
 */

static void initUpStreamTask()
{
	int err = 0;
	UpStreamMsgQ = NULL;

	err = pthread_create(&UpStreamMsgThreadId, NULL, handle_upstream, NULL);
	if (err != 0) 
	{
		dbg_err (err, "Error creating messages threadn");
	}
	else
	{
		printf("handle_upstream thread created Successfully\n");
	}
}

/*
 * @brief To handle UpStream messages received from parodus lib
 *        going up to parodus service.
 *        Reads messages from nanomsg socket and puts them
 *        on UpStreamMsgQ         
 */

static void *handle_upstream()
{

	printf("******** Start of handle_upstream ********\n");
	
	UpStreamMsg *message;
	int sock;
	int bytes =0;
	void *buf;
		
		
	sock = nn_socket( AF_SP, NN_PULL );
	nn_bind(sock, PARODUS_UPSTREAM );
	
	
	while( 1 ) 
	{
		
		buf = NULL;
		printf("nanomsg server gone into the listening mode...\n");
		
		bytes = nn_recv (sock, &buf, NN_MSG, 0);
			
		printf ("Upstream message received from nanomsg client: \"%s\"\n", (char*)buf);
		
		message = (UpStreamMsg *)malloc(sizeof(UpStreamMsg));
		
		if(message)
		{
			message->msg =buf;
			message->len =bytes;
			message->next=NULL;
			pthread_mutex_lock (&nano_mut); // was nano_prod_mut
			
			//Producer adds the nanoMsg into queue
			if(UpStreamMsgQ == NULL)
			{
	
				UpStreamMsgQ = message;
				
				printf("UpStreamMsgQ producer added message\n");
			 	pthread_cond_signal(&nano_con);
				pthread_mutex_unlock (&nano_mut); // was nano_prod_mut
				printf("mutex unlock in UpStreamMsgQ producer thread\n");
			}
			else
			{
				UpStreamMsg *temp = UpStreamMsgQ;
				while(temp->next)
				{
					temp = temp->next;
				}
			
				temp->next = message;
			
				pthread_mutex_unlock (&nano_mut); // was nano_prod_mut
			}
					
		}
		else
		{
			printf("failure in allocation for message\n");
		}
				
	}
	printf ("End of handle_upstream\n");
	return 0;
}


/*
 * @brief To process the received msg and to send upstream
 */

static void processUpStreamTask()
{
	int err = 0;
	UpStreamMsgQ = NULL;

	err = pthread_create(&processUpStreamThreadId, NULL, processUpStreamHandler, NULL);
	if (err != 0) 
	{
		dbg_err (err, "Error creating messages thread\n");
	}
	else
	{
		printf("processUpStreamHandler thread created Successfully\n");
	}
}



static void *processUpStreamHandler()
{
	printf("Inside processUpStreamHandler..\n");
	handleUpStreamEvents();
	return 0;
}

/*
 * @brief To read and process messages on UpStreamMsgQ ,
 *        which came from the parodus lib.
 */

static void handleUpStreamEvents()
{		
	int rv=-1;	
	int msgType;
	wrp_msg_t *msg;		
	int i =0;	
	int size=0;
	int matchFlag = 0;	
	int byte = 0;	
	void *auth_bytes;
	wrp_msg_t auth_msg_var;
	
	auth_msg_var.msg_type = WRP_MSG_TYPE__AUTH;
	auth_msg_var.u.auth.status = 200;
		
	while(1)
	{
		pthread_mutex_lock (&nano_mut); // was nano_cons_mut
		printf("mutex lock in UpStreamMsgQ consumer thread\n");
		
		if(UpStreamMsgQ != NULL)
		{
			UpStreamMsg *message = UpStreamMsgQ;
			UpStreamMsgQ = UpStreamMsgQ->next;
			pthread_mutex_unlock (&nano_mut); // was nano_cons_mut
			printf("mutex unlock in UpStreamMsgQ consumer thread\n");
			
			if (!terminated) 
			{
				
				/*** Decoding Upstream Msg to check msgType ***/
				/*** For MsgType 9 Perform Nanomsg client Registration else Send to server ***/	
				
				printf("---- Decoding Upstream Msg ----\n");
								
				rv = wrp_to_struct( message->msg, message->len, WRP_BYTES, &msg );
				
				if(rv > 0)
				{
				
				   msgType = msg->msg_type;				   
				
				   if(msgType == 9)
				   {
					printf("\n Nanomsg client Registration for Upstream\n");
					
					//Extract serviceName and url & store it in a struct for reg_clients
					
					if(numOfClients !=0)
					{
					    for( i = 0; i < numOfClients; i++ ) 
					    {
																						
						if(strcmp(clients[i]->service_name, msg->u.reg.service_name)==0)
						{
							printf("match found, client is already registered\n");
							strcpy(clients[i]->url,msg->u.reg.url);
							nn_shutdown(clients[i]->sock, 0);


							clients[i]->sock = nn_socket( AF_SP, NN_PUSH );
							int t = 20000;
							nn_setsockopt(clients[i]->sock, NN_SOL_SOCKET, NN_SNDTIMEO, &t, sizeof(t));
							
							nn_connect(clients[i]->sock, msg->u.reg.url);  
			
							
							size = wrp_struct_to( &auth_msg_var, WRP_BYTES, &auth_bytes );
							printf("Client registered before. Sending acknowledgement \n");
							byte = nn_send (clients[i]->sock, auth_bytes, size, 0);
						
							byte = 0;
							size = 0;
							matchFlag = 1;
							break;
						}
					    }

					}

					printf("matchFlag is :%d\n", matchFlag);

					if((matchFlag == 0) || (numOfClients == 0))
					{
					
						clients[numOfClients] = (reg_client*)malloc(sizeof(reg_client));

						clients[numOfClients]->sock = nn_socket( AF_SP, NN_PUSH );
						nn_connect(clients[numOfClients]->sock, msg->u.reg.url);  

						int t = 20000;
						nn_setsockopt(clients[numOfClients]->sock, NN_SOL_SOCKET, NN_SNDTIMEO, &t, sizeof(t));
						 
						strcpy(clients[numOfClients]->service_name,msg->u.reg.service_name);
						strcpy(clients[numOfClients]->url,msg->u.reg.url);

						printf("%s\n",clients[numOfClients]->service_name);
						printf("%s\n",clients[numOfClients]->url);
						
						if((strcmp(clients[numOfClients]->service_name, msg->u.reg.service_name)==0)&& (strcmp(clients[numOfClients]->url, msg->u.reg.url)==0))
					{


						//Sending success status to clients after each nanomsg registration
						size = wrp_struct_to(&auth_msg_var, WRP_BYTES, &auth_bytes );

						printf("Client %s Registered successfully. Sending Acknowledgement... \n ", clients[numOfClients]->service_name);

						byte = nn_send (clients[numOfClients]->sock, auth_bytes, size, 0);
						
						//condition needs to be changed depending upon acknowledgement
						if(byte >=0)
						{
							printf("send registration success status to client\n");
						}
						else
						{
							printf("send registration failed\n");
						}
						numOfClients =numOfClients+1;
						printf("Number of clients registered= %d\n", numOfClients);
						byte = 0;
						size = 0;
						free(auth_bytes);

						
					
					}
					else
					{
						printf("nanomsg client registration failed\n");
					}
					
					}
										      

				    }
				    else
				    {
				    	//Sending to server for msgTypes 3, 4, 5, 6, 7, 8.
					
			   					
							printf("\n Received upstream data with MsgType: %d\n", msgType);   					
							//Appending metadata with packed msg received from client
					   	handleUpstreamMessage(msg);
					
				    
				    }
					wrp_free_struct (msg);
				}
				else
				{
					printf("Error in msgpack decoding for upstream\n");
				
				}
				
			
			}
		
			
			nn_freemsg (message->msg);
			free(message);
			message = NULL;
		}
		else
		{
			printf("Before pthread cond wait in UpStreamMsgQ consumer thread\n");   
			pthread_cond_wait(&nano_con, &nano_mut); // was nano_prod_mut
			pthread_mutex_unlock (&nano_mut); // was nano_cons_mut
			printf("mutex unlock in UpStreamMsgQ consumer thread after cond wait\n");
			if (terminated) {
				break;
			}
		}
	}

} // End handleUpStreamEvents
 
 
        
/*
 * @brief To initiate message handling
 */

static void initMessageHandler()
{
	int err = 0;
	ParodusMsgQ = NULL;

	err = pthread_create(&messageThreadId, NULL, messageHandlerTask, NULL);
	if (err != 0) 
	{
		dbg_err (err, "Error creating messages thread\n");
	}
	else
	{
		printf("messageHandlerTask thread created Successfully\n");
	}
}

/*
 * @brief To read and process messages on the ParodusMsgQ
 *        In the real parodus service, these messages come from webpa
 *        In mock_parodus, they come from a test file.  
 */
static void *messageHandlerTask()
{
	int p;

	while(1)
	{
		pthread_mutex_lock (&parodus_mut);
		printf("mutex lock in ParodusMsgQ consumer thread\n");
		if(ParodusMsgQ != NULL)
		{
			int rtn;
			ParodusMsg *message = ParodusMsgQ;
			ParodusMsgQ = ParodusMsgQ->next;
			pthread_mutex_unlock (&parodus_mut);
			printf("mutex unlock in ParodusMsgQ consumer thread\n");
			rtn = listenerOnMessage(message->payload, message->len);
			free(message);
			message = NULL;
			if (rtn == 1)
				break;
		}
		else
		{
			printf("Before pthread cond wait in ParodusMsgQ consumer thread\n");   
			pthread_cond_wait(&parodus_con, &parodus_mut);
			pthread_mutex_unlock (&parodus_mut);
			printf("mutex unlock in ParodusMsgQ consumer thread after cond wait\n");
		}
	}
	
	printf ("Ended messageHandlerTask\n");
	for( p = 0; p < numOfClients; p++ ) 
		nn_shutdown(clients[p]->sock, 0);
	return 0;
} // End messageHandlerTask

static char *new_str (const char *str)
{
	char *buf = malloc (strlen(str) + 1);
	if (NULL == buf)
		return NULL;
	strcpy (buf, str);
	return buf;
}

static void insert_number_into_buf (char *buf, unsigned num)
{
	char *pos = strrchr (buf, '#');
	if (NULL == pos)
		return;
	while (true) {
		*pos = (num%10) + '0';
		num /= 10;
		if (pos <= buf)
			break;
		pos--;
		if (*pos != '#')
			break;
	}
}

static int enqueue_test_msg (const char *trans_uuid, unsigned trans_num, 
	const char *src, const char *dest, const char *payload)
{
	ssize_t msg_len;
	void *msg_bytes;
	char *trans_buf;
	wrp_msg_t *new_msg = malloc (sizeof (wrp_msg_t));
	if (NULL == new_msg)
		return -1;
	printf ("Making req msg\n");
	new_msg->msg_type = WRP_MSG_TYPE__REQ;
	trans_buf = new_str (trans_uuid);
	insert_number_into_buf (trans_buf, trans_num);
	new_msg->u.req.transaction_uuid = (void*) trans_buf;
	new_msg->u.req.source = new_str (src);
	new_msg->u.req.dest = new_str (dest);
	new_msg->u.req.headers = NULL;
	new_msg->u.req.metadata = NULL;
	new_msg->u.req.payload = new_str (payload);
	new_msg->u.req.payload_size = strlen (payload) + 1;
	printf ("Enqueueing req msg to parodus lib\n");
	msg_len = wrp_struct_to (new_msg, WRP_BYTES, &msg_bytes);
	if (msg_len < 1) {
		printf ("LIBPARODUS: error converting WRP to bytes\n");
		return -1;
	}
	listenerOnMessage_queue (msg_bytes, msg_len);
	//printf ("Freeing event msg\n");
	wrp_free_struct (new_msg);
	//printf ("Freed event msg\n");
	return 0;
}

/**
 * @brief listenerOnMessage_queue enqueue a message on to the ParodusMsgQ
 *
 */

static void listenerOnMessage_queue(void *msg_bytes, ssize_t msg_len)
{
	ParodusMsg *message;

	if (terminated) 
	{
		return;
	}

	message = (ParodusMsg *)malloc(sizeof(ParodusMsg));

	if(message)
	{

		//message->msg = msg;
		message->payload = msg_bytes;
		message->len = msg_len;
		message->next = NULL;

		pthread_mutex_lock (&parodus_mut);		
		printf("mutex lock in ParodusMsgQ producer thread\n");
		
		if(ParodusMsgQ == NULL)
		{
			ParodusMsgQ = message;
			printf("ParodusMsgQ producer added message\n");
		 	pthread_cond_signal(&parodus_con);
			pthread_mutex_unlock (&parodus_mut);
			printf("mutex unlock in ParodusMsgQ producer thread\n");
		}
		else
		{
			ParodusMsg *temp = ParodusMsgQ;
			while(temp->next)
			{
				temp = temp->next;
			}
			temp->next = message;
			pthread_mutex_unlock (&parodus_mut);
		}
	}
	else
	{
		//Memory allocation failed
		printf("Allocation of ParodusMsg failed in listenerOnMessageQueue\n");
	}
	printf("*****Returned from listenerOnMessage_queue*****\n");
} // End listenerOnMessage_queue

/**
 * @brief listenerOnMessage function to process a message
 * 				on the ParodusMsgQ, sending it via nanomsg
 *				to the parodus lib.
 *
 */
static int listenerOnMessage(void * msg, size_t msgSize)
{
	
	int rv =0;
	wrp_msg_t *message;
	char* destVal = NULL;
	char dest[32] = {'\0'};
	
	int msgType;
	int p =0;
	int bytes =0;
	int destFlag =0;	
	const char *recivedMsg = NULL;
	recivedMsg =  (const char *) msg;
	
	printf("Received msg from server:%s\n", recivedMsg);	
	if(recivedMsg!=NULL) 
	{
	
		/*** Decoding downstream recivedMsg to check destination ***/
		
		rv = wrp_to_struct(recivedMsg, msgSize, WRP_BYTES, &message);
				
		if(rv > 0)
		{
			printf("\nDecoded recivedMsg of size:%d\n", rv);
			msgType = message->msg_type;
			printf("msgType received:%d\n", msgType);
		
			if((message->u.req.dest !=NULL))
			{
				destVal = message->u.req.dest;
				if (strcmp (destVal, "END") == 0) {
					wrp_free_struct (message);
					return 1;
				}
				strtok(destVal , "/");
				strcpy(dest,strtok(NULL , "/"));
				printf("Received downstream dest as :%s\n", dest);
			
				//Checking for individual clients & Sending to each client
				
				for( p = 0; p < numOfClients; p++ ) 
				{
				    // Sending message to registered clients
				    if( strcmp(dest, clients[p]->service_name) == 0) 
				    {  
				    	printf("sending to nanomsg client %s\n", dest);     
							bytes = nn_send(clients[p]->sock, recivedMsg, msgSize, 0);
							printf("sent downstream message '%s' to reg_client '%s'\n",recivedMsg,clients[p]->url);
							printf("downstream bytes sent:%d\n", bytes);
							destFlag =1;
					 } 
				}
				
				if(destFlag ==0)
				{
					printf("Unknown dest:%s\n", dest);
				}
			
			}
				wrp_free_struct (message);
	  }
	  	
	  else
	  {
	  	printf( "Failure in msgpack decoding for receivdMsg: rv is %d\n", rv );
			return -1;
	  }
	  
	}
  return 0;              
     
} // End listenerOnMessage


/*
 * @brief displays the current time.
 * @param[in] timer current time.
 */
void getCurrentTime(struct timespec *timer)
{
	clock_gettime(CLOCK_REALTIME, timer);
}
/*
 * @brief displays the current time in microseconds.
 * @param[in] timer current time.
 */

uint64_t getCurrentTimeInMicroSeconds(struct timespec *timer)
{
        uint64_t systime = 0;
	    clock_gettime(CLOCK_REALTIME, timer);       
        printf("timer->tv_sec : %lu\n",timer->tv_sec);
        printf("timer->tv_nsec : %lu\n",timer->tv_nsec);
        systime = (uint64_t)timer->tv_sec * 1000000L + timer->tv_nsec/ 1000;
        return systime;	
}

/*
 * @brief Returns the time difference between start and end time of request processed.
 * @param[in] starttime starting time of request processed.
 * @param[in] finishtime ending time of request processed.
 * @return msec.
 */
long timeValDiff(struct timespec *starttime, struct timespec *finishtime)
{
	long msec;
	msec=(finishtime->tv_sec-starttime->tv_sec)*1000;
	msec+=(finishtime->tv_nsec-starttime->tv_nsec)/1000000;
	return msec;
}



void parStrncpy(char *destStr, const char *srcStr, size_t destSize)
{
    strncpy(destStr, srcStr, destSize-1);
    destStr[destSize-1] = '\0';
}

static int convert_num (char *arg, const char *arg_name, 
	unsigned long *num, unsigned long min, unsigned long max)
{
	char *endarg;
	*num = strtoul (arg, &endarg, 10);
	if ((errno == ERANGE) || (*endarg != '\0')) {
		printf ("Invalid %s arg %s\n", arg_name, arg);
		return -1;
	} 
	if ((*num < min) || (*num > max)) {
		printf ("arg %s out of range (%lu..%lu)\n", arg_name, min, max);
		return -1;
	}
	return 0; 
}

static int parseCommandLine(int argc,char **argv,Cfg_t * cfg)
{
    
  int c;
  static struct option long_options[] = {
     {"test-file",  required_argument, 0, 'f'},
     {"delay",  required_argument, 0, 'd'},
     {"msg-count",  optional_argument, 0, 'c'},
		 {"create-pipe", optional_argument, 0, 'p'},
     {0, 0, 0, 0}
  };

	memset(cfg,0,sizeof(Cfg_t));
    while (1)
    {
      /* getopt_long stores the option index here. */
      int option_index = 0;
      c = getopt_long (argc, argv, "f:d:c:",long_options, &option_index);

      /* Detect the end of the options. */
      if (c == -1)
        break;

      switch (c)
        {
        case 'f':
          strncpy(cfg->test_msgs_file, optarg,strlen(optarg));
          printf("test_msgs_file is %s\n",cfg->test_msgs_file);
          break;
        case 'd':
					if (convert_num (optarg, "test_msg_delay", &cfg->test_msg_delay,
							0, 10) == 0)
            break;
					return -1;
        case 'c':
					if (convert_num (optarg, "test_msg_count", &cfg->test_msg_count,
							0, 500) == 0)
            break;
					return -1;
				case 'p':
					if (convert_num (optarg, "create_pipe_opt", &cfg->create_pipe_opt,
							0,1) == 0)
						break;
					return -1;
        case '?':
          /* getopt_long already printed an error message. */
          break;

        default:
          printf("Enter Valid commands..\n");
          return -1;
        }
    }
  
 printf("argc is :%d\n", argc);
 printf("optind is :%d\n", optind);

  /* Print any remaining command line arguments (not options). */
  if (optind < argc)
    {
      printf ("non-option ARGV-elements: ");
      while (optind < argc)
        printf ("%s ", argv[optind++]);
      putchar ('\n');
    }
	return 0;
} // End parseCommandLine

void show_src_dest_payload (char *src, char *dest, void *payload, size_t payload_size)
{
	size_t i;
	char *payload_str = (char *) payload;
	printf (" SOURCE:  %s\n", src);
	printf (" DEST  :  %s\n", dest);
	printf (" PAYLOAD: ");
	for (i=0; i<payload_size; i++)
		putchar (payload_str[i]);
	putchar ('\n');
}

void show_wrp_req_msg (struct wrp_req_msg *msg)
{
	show_src_dest_payload (msg->source, msg->dest, msg->payload, msg->payload_size);
}

void show_wrp_event_msg (struct wrp_event_msg *msg)
{
	show_src_dest_payload (msg->source, msg->dest, msg->payload, msg->payload_size);
}

void show_wrp_msg (wrp_msg_t *wrp_msg)
{
	printf ("Received WRP Msg type %d\n", wrp_msg->msg_type);
	if (wrp_msg->msg_type == WRP_MSG_TYPE__REQ) {
		show_wrp_req_msg (&wrp_msg->u.req);
		return;
	}
	if (wrp_msg->msg_type == WRP_MSG_TYPE__EVENT) {
		show_wrp_event_msg (&wrp_msg->u.event);
		return;
	}
	return;
}

static int get_trans_num (wrp_msg_t *msg)
{
	unsigned trans = 0;
	int i;
	char fc, mc;
	const char *msg_trans;

	if (msg->msg_type != WRP_MSG_TYPE__REQ)
		return -1;
	msg_trans = msg->u.req.transaction_uuid;
	for (i=0; (fc=trans_format[i]) != 0; i++)
	{
		mc = msg_trans[i];
		if (mc == 0)
			return -1;
		if (fc == '#') {
			if ((mc >= '0') && (mc <= '9')) {
				trans = 10*trans + (mc - '0');
				continue;
			}
			return -1;
		}
		if (fc != mc)
			return -1;
	}
	if (msg_trans[i] != 0)
		return -1;
	return (int) trans;
}

/** To send upstream msgs to server ***/
static void handleUpstreamMessage(wrp_msg_t *msg)
{
	int trans_num;
	show_wrp_msg (msg);
	if (msg->msg_type != WRP_MSG_TYPE__REQ)
		return;
	trans_num = get_trans_num (msg);
	if (trans_num < 0) {
		printf ("Invalid transaction uuid in REQ msg\n");
		return;
	}
	pthread_mutex_lock (&reply_mut);
	if ((unsigned) trans_num != reply_trans) {
		pthread_mutex_unlock (&reply_mut);
		printf ("Unmatched transaction uuid in REQ msg\n");
		return;
	}
	printf ("Mock Parodus Got reply to trans %u on UpStreamMsgQ\n", trans_num);
	reply_trans = 0;
	pthread_cond_signal (&reply_con);
	pthread_mutex_unlock (&reply_mut);
}

static int rewind_test_msg_file (void)
{
	if (fseek (test_msgs_fp, 0, SEEK_SET) < 0) {
		printf ("Error seeking pos 0 in req test msgs file\n");
		return -1;
	}
	return 0;
}

static int test_file_read (char *buf, char **dest, char **payload)
{
	char *endpos;
	char *space_pos;
	const char *errmsg = "Format error in test msgs file";

	*dest = NULL;
	*payload = NULL;
	if (NULL == fgets (buf, TEST_MSG_BUF_LEN, test_msgs_fp))
		return 1;
	endpos = strchr (buf, '\n');
	if (NULL == endpos) {
		puts (errmsg);
		return -1;
	}
	*endpos = '\0';
	space_pos = strchr (buf, ' ');
	if (NULL == space_pos) {
		puts (errmsg);
		return -1;
	}
	*space_pos = '\0';
	if (space_pos == buf) {
		puts (errmsg);
		return -1;
	}
	*dest = buf;
	*payload = space_pos+1;
	return 0;
}

static int wait_end_pipe_msg (time_t nsecs)
{
	struct timeval tv;
	int rtn;
	int nfds = end_pipe_fd + 1;
	fd_set readfds;
	tv.tv_sec = nsecs;
	tv.tv_usec = 0;
	FD_ZERO (&readfds);
	FD_SET (end_pipe_fd, &readfds);
	rtn = select (nfds, &readfds, NULL, NULL, &tv);
	if (rtn == 0)
		return 0;
	if (rtn == -1) {
		dbg_err (errno, "Error on select\n");
		return -1;
	}
	return read_end_pipe ();
}

int get_expire_time (uint32_t ms, struct timespec *ts)
{
	struct timeval tv;
	int err = gettimeofday (&tv, NULL);
	if (err != 0) {
		dbg_err (errno, "Error getting time of day\n");
		return err;
	}
	tv.tv_sec += ms/1000;
	tv.tv_usec += (ms%1000) * 1000;
	if (tv.tv_usec >= 1000000) {
		tv.tv_sec += 1;
		tv.tv_usec -= 1000000;
	}

	ts->tv_sec = tv.tv_sec;
	ts->tv_nsec = tv.tv_usec * 1000L;
	return 0;
}

static int wait_trans (unsigned trans_num)
{
	int rtn;
	struct timespec ts;

	pthread_mutex_lock (&reply_mut);
	reply_trans = trans_num;

	while (true) {
		rtn = get_expire_time (20000, &ts);
		if (rtn != 0)
			return -1;
		rtn = pthread_cond_timedwait (&reply_con, &reply_mut, &ts);
		if (rtn == ETIMEDOUT) {
			pthread_mutex_unlock (&reply_mut);
			printf ("Mock Parodus Timed out waiting for reply to trans %u\n", trans_num);
			return 1;
		}
		if (reply_trans == 0) {
			pthread_mutex_unlock (&reply_mut);
			printf ("Mock Parodus Got reply to trans %u\n", trans_num);
			return 0;
		}
		pthread_mutex_unlock (&reply_mut);
		pthread_mutex_lock (&reply_mut);
	}
}

static void read_and_send_test_msgs (void)
{
	unsigned trans_num = 0;
	const char *source = "---MOCK_PARODUS---";
	char file_buf[TEST_MSG_BUF_LEN];
	char *dest;
	char *payload;
	int rtn;

	while (1) {
		rtn = test_file_read (file_buf, &dest, &payload);
		if (rtn == 1) {
			if (trans_num == 0)
				break;
			rewind_test_msg_file ();
			continue;
		}
		if (rtn == -1)
			break;
		trans_num++;
		enqueue_test_msg (trans_format, trans_num, source, dest, payload);
		if (wait_trans (trans_num) != 0)
			break;
		if ((Cfg.test_msg_count != 0) && (trans_num >= Cfg.test_msg_count))
			break;
		if (wait_end_pipe_msg (Cfg.test_msg_delay) != 0)
			break;
	}
	trans_num++;
	enqueue_test_msg (trans_format, trans_num, source, "END", "END");
	fclose (test_msgs_fp);
	close (end_pipe_fd);
}

int main( int argc, char **argv)
{
	if (parseCommandLine(argc,argv,&Cfg) != 0)
		return 4;
	test_msgs_fp = fopen (Cfg.test_msgs_file, "r");
	if (NULL == test_msgs_fp) {
		dbg_err (errno, "Error opening file %s\n", Cfg.test_msgs_file);
		return 4;
	} 
	if (open_end_pipe( (bool) Cfg.create_pipe_opt) != 0)
		return 4;      
	initTasks(NULL);
	read_and_send_test_msgs ();
	terminateTasks ();

	return 0;
}
