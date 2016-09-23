#include <stdio.h>
#include <string.h>
#include <nanomsg/nn.h>
#include <nanomsg/pipeline.h>

const char *rcv_id = NULL;
const char *send_id = NULL;
const char *msg = NULL;

int main (const int argc, const char **argv)
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
		mode = 0;
	}

  printf ("Done!\n");
}
