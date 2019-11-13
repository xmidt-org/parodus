/**
 * Copyright 2015 Comcast Cable Communications Management, LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
/**
 * @file client_list.h
 *
 * @description This file is used to manage registered clients
 *
 */

#include "ParodusInternal.h"
#include "connection.h"
#include "client_list.h"

/*----------------------------------------------------------------------------*/
/*                            File Scoped Variables                           */
/*----------------------------------------------------------------------------*/
static int numOfClients = 0;
static reg_list_item_t * g_head = NULL;
pthread_mutex_t client_mut=PTHREAD_MUTEX_INITIALIZER;

/*----------------------------------------------------------------------------*/
/*                             External functions                             */
/*----------------------------------------------------------------------------*/

reg_list_item_t * get_global_node(void)
{
    pthread_mutex_lock (&client_mut);
    return g_head;
}

void release_global_node (void)
{
    pthread_mutex_unlock (&client_mut);
}


int get_numOfClients()
{
    return numOfClients;
}
/** To add clients to registered list ***/

int addToList( wrp_msg_t **msg)
{   
    //new_node indicates the new clients which needs to be added to list
    int rc = -1;
    int sock;
    int retStatus = -1;

    sock = nn_socket( AF_SP, NN_PUSH );
    ParodusPrint("sock created for adding entries to list: %d\n", sock);
    if(sock >= 0)
    {
            int t = NANO_SOCKET_SEND_TIMEOUT_MS;
            rc = nn_setsockopt(sock, NN_SOL_SOCKET, NN_SNDTIMEO, &t, sizeof(t));
            if(rc < 0)
            {
                ParodusError ("Unable to set socket timeout (errno=%d, %s)\n",errno, strerror(errno));
            }
            
            rc = nn_connect(sock, (*msg)->u.reg.url);
            if(rc < 0)
            {
                ParodusError ("Unable to connect socket (errno=%d, %s)\n",errno, strerror(errno));
		if (nn_close (sock) < 0)
                {
                   ParodusError ("nn_close socket=%d (err=%d, %s)\n", 
			sock, errno, strerror(errno));
		}
                
            }
            else
            {
            	reg_list_item_t *new_node = NULL;
		new_node=(reg_list_item_t *)malloc(sizeof(reg_list_item_t));
		if(new_node)
		{
    			memset( new_node, 0, sizeof( reg_list_item_t ) );
    			new_node->sock = sock;
                        new_node->endpoint = rc;
    			ParodusPrint("new_node->sock is %d\n", new_node->sock);
    			
    			
	                ParodusPrint("(*msg)->u.reg.service_name is %s\n", (*msg)->u.reg.service_name);
	                ParodusPrint("(*msg)->u.reg.url is %s\n", (*msg)->u.reg.url);

	                parStrncpy(new_node->service_name, (*msg)->u.reg.service_name, sizeof(new_node->service_name));
	                parStrncpy(new_node->url, (*msg)->u.reg.url, sizeof(new_node->url));
	                new_node->next=NULL;
	                 
	                if (g_head == NULL) //adding first client
	                {
	                        ParodusInfo("Adding first client to list\n");
	                        g_head = new_node;
	                }
	                else   //client2 onwards           
	                {
	                        reg_list_item_t *temp = NULL;
	                        ParodusInfo("Adding clients to list\n");
	                        temp = g_head;

	                        while(temp->next !=NULL)
	                        {
		                        temp=temp->next;
	                        }

	                        temp->next=new_node;
	                }

	                ParodusPrint("client is added to list\n");
	                ParodusInfo("client service %s is added to list with url: %s\n", new_node->service_name, new_node->url);
	                if((strcmp(new_node->service_name, (*msg)->u.reg.service_name)==0)&& (strcmp(new_node->url, (*msg)->u.reg.url)==0))
	                {
	                	numOfClients = numOfClients + 1;
	                        ParodusInfo("sending auth status to reg client\n");
	                        retStatus = sendAuthStatus(new_node);
	                }
	                else
	                {
	                        ParodusError("nanomsg client registration failed\n");
	                        
	                }
	            }
            }
    }
    else
    {
            ParodusError("Unable to create socket (errno=%d, %s)\n",errno, strerror(errno));
            
    }
    ParodusPrint("addToList return %d\n", retStatus);
    return retStatus;
   
}

/** To send auth status to clients in list ***/

int sendAuthStatus(reg_list_item_t *new_node)
{
	int byte = 0, nbytes = -1;	
	size_t size=0;
	void *auth_bytes;
	wrp_msg_t auth_msg_var;
	int status = -1;
	
	auth_msg_var.msg_type = WRP_MSG_TYPE__AUTH;
	auth_msg_var.u.auth.status = 200;
	
	//Sending success status to clients after each nanomsg registration
	nbytes = wrp_struct_to(&auth_msg_var, WRP_BYTES, &auth_bytes );
        if(nbytes < 0)
        {
                ParodusError(" Failed to encode wrp struct returns %d\n", nbytes);
        }
        else
        {
	        ParodusInfo("Client %s Registered successfully. Sending Acknowledgement... \n ", new_node->service_name);
                size = (size_t) nbytes;
                ParodusInfo("Sending ack:new_node->sock %d service:%s\n", new_node->sock, new_node->service_name);
	        byte = nn_send (new_node->sock, auth_bytes, size, 0);
		
	        if(byte >=0)
	        {
	            ParodusPrint("send registration success status to client\n");
	            status = 0;
	        }
	        else
	        {
	            ParodusError("send registration failed\n");
	        }
        }
	byte = 0;
	size = 0;
	free(auth_bytes);
	auth_bytes = NULL;
	return status;
}

/** To delete clients from registered list ***/    
     
int deleteFromList(char* service_name)
{
 	reg_list_item_t *prev_node = NULL, *curr_node = NULL;

	if( NULL == service_name ) 
	{
		ParodusError("Invalid value for service\n");
		return -1;
	}
	ParodusInfo("service to be deleted: %s\n", service_name);

	prev_node = NULL;
	curr_node = g_head ;	

	// Traverse to get the link to be deleted
	while( NULL != curr_node )
	{
		if(strcmp(curr_node->service_name, service_name) == 0)
		{
			ParodusPrint("Found the node to delete\n");
			if( NULL == prev_node )
			{
				ParodusPrint("need to delete first client\n");
			 	g_head = curr_node->next;
			}
			else
			{
				ParodusPrint("Traversing to find node\n");
			 	prev_node->next = curr_node->next;
			}
			
			ParodusPrint("Deleting the node\n");
                        if(nn_shutdown(curr_node->sock, curr_node->endpoint) < 0)
                        {
                           ParodusError ("nn_shutdown socket=%d endpt=%d, err=%d\n", 
				curr_node->sock, curr_node->endpoint, errno);
                        }
			if (nn_close (curr_node->sock) < 0)
                        {
                           ParodusError ("nn_close socket=%d err=%d\n", 
				curr_node->sock, errno);
                        }
			free( curr_node );
			curr_node = NULL;
			ParodusInfo("Deleted successfully and returning..\n");
			numOfClients =numOfClients - 1;
			ParodusPrint("numOfClients after delete is %d\n", numOfClients);
			return 0;
		}
		
		prev_node = curr_node;
		curr_node = curr_node->next;
	}
	ParodusError("Could not find the entry to delete from list\n");
	return -1;
}

void deleteAllClients (void)
{
  reg_list_item_t *next_node = NULL;

  while (NULL != g_head)
  {
    next_node = g_head->next;
    free (g_head);
    g_head = next_node;
  }
  if (numOfClients > 0) {
    ParodusInfo ("Deleted %d clients\n", numOfClients);
    numOfClients = 0;
  }
}

/*
*@dest : Client destination to send message
*@Msg:	Msg to send it to client (No free done here), user responsibilites to free the msg
*@msgSize : Total size of the msg to send to client
*/
int sendMsgtoRegisteredClients(char *dest,const char **Msg,size_t msgSize)
{
	int bytes =0;
	reg_list_item_t *temp = NULL;
	temp = get_global_node();
	//Checking for individual clients & Sending msg to registered client
	while (NULL != temp)
	{
		ParodusPrint("node is pointing to temp->service_name %s \n",temp->service_name);
		// Sending message to registered clients
		if( strcmp(dest, temp->service_name) == 0)
		{
			bytes = nn_send(temp->sock, *Msg, msgSize, 0);
			release_global_node ();
			ParodusInfo("sent downstream message to reg_client '%s'\n", temp->url);
			ParodusPrint("downstream bytes sent:%d\n", bytes);
			return 1;
		}
		ParodusPrint("checking the next item in the list\n");
		temp= temp->next;
	}
	release_global_node ();
	return 0;
}
