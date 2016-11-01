#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <nanomsg/nn.h>
#include <nanomsg/pipeline.h>

const char *rcv_id = NULL;
const char *send_id = NULL;
const char *msg = NULL;
char rcv_url[128];
char send_url[128];
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
	if (rcv_sock != -1)
		nn_shutdown (rcv_sock, 0);
}

int connect_sender (void)
{
	int sock;
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
	if (send_sock != -1)
		nn_shutdown (send_sock, 0);
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
		printf ("arg not preceded by r/s/m specifier\n");
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

void wait_for_msgs (void)
{
	char *my_msg;
	char reply_buf[128];
	while (1)
	{
		my_msg = receive_msg();
		if (NULL == my_msg)
			break; 
		if (send_sock == -1)
			continue;
		sprintf (reply_buf, "REPLY: %s", my_msg);
		nn_freemsg (my_msg);
		send_msg (reply_buf);
	}
	shutdown_sender ();
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

	if (NULL != rcv_id)
		if (connect_receiver () < 0)
			exit(4);

	if (NULL != send_id)
		if (connect_sender () < 0)
			exit(4);

	if (NULL == msg)
		wait_for_msgs ();
	else
		send_command ();

  printf ("Done!\n");
}
