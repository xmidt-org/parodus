/**
 * @file internal.c
 *
 * @description This file is used to manage internal functions of parodus
 *
 * Copyright (c) 2015  Comcast
 */

#include "ParodusInternal.h"
#include "wss_mgr.h"
#include "connection.h"
#include "client_list.h"
#include <nanomsg/nn.h>
#include <nanomsg/pipeline.h>


static reg_list_item_t * g_head = NULL;

/*----------------------------------------------------------------------------*/
/*                             Internal functions                             */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/*                             External functions                             */
/*----------------------------------------------------------------------------*/

reg_list_item_t * get_global_node(void)
{
    return g_head;
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
            int t = NANOMSG_SOCKET_TIMEOUT_MSEC;
            rc = nn_setsockopt(sock, NN_SOL_SOCKET, NN_SNDTIMEO, &t, sizeof(t));
            if(rc < 0)
            {
                ParodusError ("Unable to set socket timeout (errno=%d, %s)\n",errno, strerror(errno));
            }
            
            rc = nn_connect(sock, (*msg)->u.reg.url);
            if(rc < 0)
            {
                ParodusError ("Unable to connect socket (errno=%d, %s)\n",errno, strerror(errno));
                nn_close (sock);
                
            }
            else
            {
            	reg_list_item_t *new_node = NULL;
		new_node=(reg_list_item_t *)malloc(sizeof(reg_list_item_t));
		if(new_node)
		{
    			memset( new_node, 0, sizeof( reg_list_item_t ) );
    			new_node->sock = sock;
    			ParodusPrint("new_node->sock is %d\n", new_node->sock);
    			
    			
	                ParodusPrint("(*msg)->u.reg.service_name is %s\n", (*msg)->u.reg.service_name);
	                ParodusPrint("(*msg)->u.reg.url is %s\n", (*msg)->u.reg.url);

	                strncpy(new_node->service_name, (*msg)->u.reg.service_name, strlen((*msg)->u.reg.service_name)+1);
	                strncpy(new_node->url, (*msg)->u.reg.url, strlen((*msg)->u.reg.url)+1);

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
			free( curr_node );
			curr_node = NULL;
			ParodusInfo("Deleted successfully and returning..\n");
			numOfClients =numOfClients - 1;
			ParodusPrint("numOfClients after delte is %d\n", numOfClients);
			return 0;
		}
		
		prev_node = curr_node;
		curr_node = curr_node->next;
	}
	ParodusError("Could not find the entry to delete from list\n");
	return -1;
}
