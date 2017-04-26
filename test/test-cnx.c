#include <stdio.h>
#include <stdio_ext.h>
#include <string.h>
#include <ev.h>
#include <webcom-c/webcom.h>

#include "stfu.h"

int stop1 = 0;
int stop2 = 0;


void test_cb(wc_event_t event, wc_cnx_t *cnx, void *data, size_t len, void *user) {
	wc_msg_t *msg = (wc_msg_t*) data;
	switch (event) {
		case WC_EVENT_ON_CNX_ESTABLISHED:
			puts("ESTABLISHED");

			break;
		case WC_EVENT_ON_MSG_RECEIVED:
			STFU_TRUE	("Receive handshake", msg->type == WC_MSG_CTRL && msg->u.ctrl.type == WC_CTRL_MSG_HANDSHAKE);
			STFU_TRUE	("Handshake path exists", msg->u.ctrl.u.handshake.server != NULL);
			printf("\t%s\n", msg->u.ctrl.u.handshake.server);

			stop1 = 1;
			break;
		case WC_EVENT_ON_CNX_CLOSED:
			puts("CNX CLOSED");
			stop2 = 1;
			break;
		default:
			break;
	}
}

ev_io stdin_watcher;
ev_timer timeout_watcher;

// all watcher callbacks have a similar signature
// this callback is called when data is readable on stdin
static void
stdin_cb (EV_P_ ev_io *w, int revents)
{
  puts ("stdin ready");
  __fpurge(stdin);
  // for one-shot events, one must manually stop the watcher
  // with its corresponding stop function.
  ev_io_stop (EV_A_ w);

  // this causes all nested ev_run's to stop iterating
  //ev_break (EV_A_ EVBREAK_ALL);
}

int main(void) {
	struct ev_loop *loop = EV_DEFAULT;
	wc_cnx_t *cnx1;

	STFU_TRUE	("Create new wc_cnx_t object", (cnx1 = wc_cnx_new("io.datasync.orange.com", 443, "/_wss/.ws?v=5&ns=legorange", loop, test_cb, (void *)0xDEADBEEF)) != NULL);
    //ev_io_init (&stdin_watcher, stdin_cb, /*STDIN_FILENO*/ 0, EV_READ);
    ev_init (&stdin_watcher, stdin_cb);
       ev_io_set (&stdin_watcher, 0, EV_READ);
    ev_io_start (loop, &stdin_watcher);

	wc_cnx_connect(cnx1);
	do {
		ev_run (loop, EVRUN_NOWAIT);
	} while (!stop1);

	wc_cnx_close(cnx1);

	do {
		ev_run (loop, 250);
	} while (!stop2);

	wc_cnx_free(cnx1);

	STFU_SUMMARY();

	return STFU_NUMBER_FAILED;
}
