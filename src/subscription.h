/**
 * @file subscription.h
 *
 * @description This header defines structures & functions required 
 * to manage parodus local client subscriptions.
 *
 * Copyright (c) 2018  Comcast
 */
 
#ifndef _SUBSCRIPTION_H_
#define _SUBSCRIPTION_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "rebar-c.h"
#include "ParodusInternal.h"
/*----------------------------------------------------------------------------*/
/*                               Data Structures                              */
/*----------------------------------------------------------------------------*/
typedef struct _Subscription {
    rebar_ll_node_t sub_node;
    char *service_name;		// Parodus local nanomsg client identifier
    char *regex;		// Interested Event subscription regex 
} Subscription;


// Struct used to represent singly linked item in a list of strings
typedef struct _StrObj {
	rebar_ll_node_t sub_node;
	char *str;
} StrObj;

/*----------------------------------------------------------------------------*/
/*                             Function Prototypes                            */
/*----------------------------------------------------------------------------*/

/* To add a client subscription to the list
 * service_name subscribed nanomsg client identifier
 * regex regular expression to match with subscribed event
 * Returns true once subscription added successfully 
 * to the in-memory subscription list otherwise false
 */
bool add_Client_Subscription(char *service_name, char *regex);

/* To retrieve subscriptions for a particular client
 * Returns a singly linked list of strings with regex
 * service_name subscribed nanomsg client identifier
 * Returns a cJSON object of various regex
 */
cJSON* get_Client_Subscriptions(char *service_name);

/* For a given event, iterate and compare with each element in list 
 * to find clients which have subscribed for this event 
 * and accordingly send to it
 * wrp_event_msg WRP event message to be matched with
 */
void filter_clients_and_send(wrp_msg_t *wrp_event_msg);


/* To delete all subscriptions for a particular client
 * service_name subscribed nanomsg client identifier
 * Deletes all entries from the in-memory subscription list
 * Returns true once subscription is deleted successfully otherwise false
 */
bool delete_client_subscriptions(char *service_name);

rebar_ll_list_t *get_global_subscription_list();

void init_subscription_list();
#ifdef __cplusplus
}
#endif


#endif /* _SUBSCRIPTION_H_ */
