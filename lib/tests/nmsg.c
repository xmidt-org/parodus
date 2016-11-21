#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdbool.h>
#include <nanomsg/nn.h>
#include <nanomsg/pipeline.h>

#define SOCK_SEND_TIMEOUT_MS 2000

const char *rcv_id = NULL;
const char *send_id = NULL;
const char *msg = NULL;
char rcv_url[128];
char send_url[128];
unsigned long send_count = 0;
bool set_timeout = true;
int rcv_sock = -1;
int send_sock = -1;
char errbuf[100];

void make_url (char *urlbuf, const char *id)
{
	if ((strlen(id) >= 3) && (strncmp (id, "tcp", 3) == 0))
		sprintf (urlbuf, "tcp://127.0.0.1:%s", id+3);
	else
		sprintf (urlbuf, "ipc:///tmp/%s.ipc", id);
}

int connect_receiver (void)
{
	int sock;
	if (NULL == rcv_id) {
		rcv_sock = -1;
		return -1;
	}
	make_url (rcv_url, rcv_id);
  sock = nn_socket (AF_SP, NN_PULL);
	if (sock < 0) {
		printf ("Unable to create rcv socket: %s\n",
			strerror_r (errno, errbuf, 100));
 		return -1;
	}
  if (nn_bind (sock, rcv_url) < 0) {
		printf ("Unable to bind to receive socket %s: %s\n",
			rcv_url, strerror_r (errno, errbuf, 100));
		return -1;
	}
	rcv_sock = sock;
	return 0;
}

void shutdown_receiver (void)
{
	if (rcv_sock != -1) {
		nn_shutdown (rcv_sock, 0);
		nn_close (rcv_sock);
	}
}

int connect_sender (void)
{
	int sock;
	int send_timeout = SOCK_SEND_TIMEOUT_MS;

	if (NULL == send_id) {
		send_sock = -1;
		return -1;
	}
	make_url (send_url, send_id);
  sock = nn_socket (AF_SP, NN_PUSH);
	if (sock < 0) {
		printf ("Unable to create send socket: %s\n",
			strerror_r (errno, errbuf, 100));
 		return -1;
	}
	if (set_timeout)
		if (nn_setsockopt (sock, NN_SOL_SOCKET, NN_SNDTIMEO, 
					&send_timeout, sizeof (send_timeout)) < 0) {
			printf ("Unable to set socket timeout: %s: %s\n",
				send_url, strerror_r (errno, errbuf, 100));
	 		return -1;
		}
  if (nn_connect (sock, send_url) < 0) {
		printf ("Unable to connect to send socket %s: %s\n",
			send_url, strerror_r (errno, errbuf, 100));
		return -1;
	}
	send_sock = sock;
	return 0;
}

void shutdown_sender (void)
{
	if (send_sock != -1) {
		nn_shutdown (send_sock, 0);
		nn_close (send_sock);
	}
}


int get_args (const int argc, const char **argv)
{
	int i;
	int mode = 0;

	for (i=1; i<argc; i++)
	{
		const char *arg = argv[i];
		if ((strlen(arg) == 1) && (arg[0] == 'r')) {
			mode = 'r';
			continue;
		}
		if ((strlen(arg) == 1) && (arg[0] == 's')) {
			mode = 's';
			continue;
		}
		if ((strlen(arg) == 1) && (arg[0] == 'm')) {
			mode = 'm';
			continue;
		}
		if ((strlen(arg) == 1) && (arg[0] == 'n')) {
			mode = 'n';
			continue;
		}
		if ((mode == 0) && (strcmp(arg, "not") == 0)) {
			set_timeout = false;
			continue;
		}
		if (mode == 'r') {
			rcv_id = arg;
			mode = 0;
			continue;
		}
		if (mode == 's') {
			send_id = arg;
			mode = 0;
			continue;
		}
		if (mode == 'm') {
			msg = arg;
			mode = 0;
			continue;
		}
		if (mode == 'n') {
			send_count = strtoul (arg, NULL, 10);
			mode = 0;
			continue;
		}
		printf ("arg not preceded by r/s/m/n specifier\n");
		return -1;
	} 
	return 0;
}

// returned msg must be freed with nn_freemsg
char *receive_msg (void)
{
	char *buf = NULL;
	int bytes;
  bytes = nn_recv (rcv_sock, &buf, NN_MSG, 0);

  if (bytes < 0) { 
		printf ("Error receiving msg: %s\n", strerror_r (errno, errbuf, 100));
		return NULL;
	}
	printf ("RECEIVED \"%s\"\n", buf);
	return buf;
}

int send_msg (const char *msg)
{
  int sz_msg = strlen (msg) + 1; // '\0' too
  int bytes;
	bytes = nn_send (send_sock, msg, sz_msg, 0);
  if (bytes < 0) { 
		printf ("Error sending msg: %s\n", strerror_r (errno, errbuf, 100));
		return -1;
	}
  if (bytes != sz_msg) {
		printf ("Not all bytes sent, just %d\n", bytes);
		return -1;
	}
	return 0;
}

void send_command (void)
{
	char *my_msg;
	if (send_msg(msg) == 0) {
		if (rcv_sock >= 0) {
			my_msg = receive_msg ();
			if (NULL != my_msg)
				nn_freemsg (my_msg);
		}
	}
	shutdown_sender ();
	shutdown_receiver ();
}

void send_multiple (void)
{
	unsigned long i;

	for (i=0; i<send_count; i++) {
		if (send_msg(msg) != 0)
			break;
		printf ("Sent msg %lu\n", i);
	}
	shutdown_sender ();
}

void wait_for_msgs (void)
{
	char *my_msg;
	char reply_buf[128];
	while (1)
	{
		my_msg = receive_msg();
		if (NULL == my_msg)
			break; 
		if (send_sock == -1) {
			nn_freemsg (my_msg);
			continue;
		}
		sprintf (reply_buf, "REPLY: %s", my_msg);
		nn_freemsg (my_msg);
		send_msg (reply_buf);
	}
	shutdown_sender ();
	shutdown_receiver ();
}

void wait_stall (void)
{
	char *my_msg;
	unsigned long count = 0;
	while (1)
	{
		if (count < send_count) {
			my_msg = receive_msg();
			if (NULL == my_msg)
				break;
			nn_freemsg (my_msg);
			count++;
			continue;
		}
		if (count == 1)
			return; 
		sleep (1);
	}
	shutdown_receiver ();
}

int main (const int argc, const char **argv)
{
	if (get_args(argc, argv) != 0)
		exit (4);

	if ((NULL != msg) && (NULL == send_id)) {
		printf ("msg specified witlhout a sender ID\n");
		exit(4);
	}

	if ((NULL == rcv_id) && (NULL == send_id)) {
		printf ("Nothing to do\n");
		exit(0);
	}

	if (NULL != rcv_id) {
		if (connect_receiver () < 0)
			exit(4);
	}

	if (NULL != send_id)
		if (connect_sender () < 0)
			exit(4);

	if (NULL == msg)
		if (send_count > 0)
			wait_stall ();
		else
			wait_for_msgs ();
	else if (send_count > 0)
		send_multiple ();
	else
		send_command ();

  printf ("Done!\n");
}
