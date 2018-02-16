/*
 * Webcom C SDK
 *
 * Copyright 2017 Orange
 * <camille.oudot@orange.com>
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include "../lib/datasync/event.c"

#include "stfu.h"

uint32_t djb2(char *str) {
	unsigned char *stru = (unsigned char *)str;
	uint32_t hash = 5381;
	int c;

	while ((c = *stru++))
		hash = ((hash << 5) + hash) + c;

	return hash;
}

unsigned c = 0;

void ev1(wc_context_t *cnx, ws_on_data_event_t event, char *path, char *json_data, void *param) {
	printf("\tcallback 1: [%p:%d:%p] %s => %s\n", cnx, event, param, path, json_data);
	if (param == (void*)0x10101010) {
		c |= 0x1;
	}
}

void ev2(wc_context_t *cnx, ws_on_data_event_t event, char *path, char *json_data, void *param) {
	printf("\tcallback 2: [%p:%d:%p] %s => %s\n", cnx, event, param, path, json_data);
	if (param == (void*)0x20202020) {
		c |= 0x2;
	}
}

void ev3(wc_context_t *cnx, ws_on_data_event_t event, char *path, char *json_data, void *param) {
	printf("\tcallback 3: [%p:%d:%p] %s => %s\n", cnx, event, param, path, json_data);
	if (param == (void*)0x30303030) {
		c |= 0x4;
	}
}

void ev4(wc_context_t *cnx, ws_on_data_event_t event, char *path, char *json_data, void *param) {
	printf("\tcallback 4: [%p:%d:%p] %s => %s\n", cnx, event, param, path, json_data);
	c |= 0x8;
}

void ev5(wc_context_t *cnx, ws_on_data_event_t event, char *path, char *json_data, void *param) {
	printf("\tcallback 5: [%p:%d:%p] %s => %s\n", cnx, event, param, path, json_data);
	c |= 0x10;
}

void ev6(wc_context_t *cnx, ws_on_data_event_t event, char *path, char *json_data, void *param) {
	printf("\tcallback 6: [%p:%d:%p] %s => %s\n", cnx, event, param, path, json_data);
	c |= 0x20;
}

int main(void)
{
	wc_context_t cnx;
	wc_push_t push;
	memset(&cnx, 0, sizeof(cnx));

	STFU_TRUE("Webcom path hash is djb2 hash", path_hash("/foo/bar/bazqux/") == djb2("/foo/bar/bazqux/"));
	STFU_TRUE("Empty path is djb2('/')", path_hash("/") == djb2("/"));
	STFU_TRUE("Empty path is djb2('/')", path_hash("") == djb2("/"));
	STFU_TRUE("Empty path is djb2('/')", path_hash("///") == djb2("/"));
	STFU_TRUE("Extra inner slashes are not significant for hashing", path_hash("/foo//////bar//bazqux/") == djb2("/foo/bar/bazqux/"));
	STFU_TRUE("Extra leading slashes are not significant for hashing", path_hash("/////foo/bar/bazqux/") == djb2("/foo/bar/bazqux/"));
	STFU_TRUE("Extra trailing slashes are not significant for hashing", path_hash("/foo/bar/bazqux/////") == djb2("/foo/bar/bazqux/"));
	STFU_TRUE("No leading slash is not significant for hashing", path_hash("foo/bar/bazqux/") == djb2("/foo/bar/bazqux/"));
	STFU_TRUE("No trailing slash is not significant for hashing", path_hash("/foo/bar/bazqux") == djb2("/foo/bar/bazqux/"));
	STFU_TRUE("Inner slash is significant for hashing", path_hash("/foo/barbazqux/") != djb2("/foo/bar/bazqux/"));

	STFU_TRUE("Extra inner slashes are not significant for equality", path_eq("/foo//////bar//bazqux/", "/foo/bar/bazqux/"));
	STFU_TRUE("Extra leading slashes are not significant for equality", path_eq("/////foo/bar/bazqux/", "/foo/bar/bazqux/"));
	STFU_TRUE("Extra trailing slashes are not significant for equality", path_eq("/foo/bar/bazqux/////", "/foo/bar/bazqux/"));
	STFU_TRUE("No leading slash is not significant for equality", path_eq("foo/bar/bazqux/", "/foo/bar/bazqux/"));
	STFU_TRUE("No trailing slash is not significant for equality", path_eq("/foo/bar/bazqux", "/foo/bar/bazqux/"));
	STFU_TRUE("Inner slash is significant for equality", path_eq("/foo/barbazqux/", "/foo/bar/bazqux/") == 0);
	STFU_TRUE("The actual content of the path is significant for equality", path_eq("/foo/bar/baZqux/", "/foo/bar/bazqux/") == 0);

	wc_datasync_route_data(&cnx, "/foo/", ev1, (void*)0x10101010);
	wc_datasync_route_data(&cnx, "/foo/bar/", ev2, (void*)0x20202020);
	wc_datasync_route_data(&cnx, "/foo/bar/baz/", ev3, (void*)0x30303030);

	wc_datasync_route_data(&cnx, "/qux/", ev4, NULL);

	wc_datasync_unroute_data(&cnx, "/qux", ev2);
	wc_datasync_unroute_data(&cnx, "/qux", ev4);

	wc_datasync_route_data(&cnx, "/qux/", ev1, (void*)1);
	wc_datasync_route_data(&cnx, "/qux/", ev2, (void*)2);
	wc_datasync_route_data(&cnx, "/qux/", ev3, (void*)3);
	wc_datasync_route_data(&cnx, "/qux/", ev3, (void*)3);
	wc_datasync_route_data(&cnx, "/qux/", ev4, (void*)4);

	wc_datasync_unroute_data(&cnx, "/qux/", ev1);
	wc_datasync_unroute_data(&cnx, "/qux/", ev2);
	wc_datasync_unroute_data(&cnx, "/qux/", ev3);
	wc_datasync_unroute_data(&cnx, "/qux/", ev4);

	wc_datasync_route_data(&cnx, "/qux/", ev1, (void*)1);
	wc_datasync_route_data(&cnx, "/qux/", ev2, (void*)2);
	wc_datasync_route_data(&cnx, "/qux/", ev3, (void*)3);
	wc_datasync_route_data(&cnx, "/qux/", ev3, (void*)3);
	wc_datasync_route_data(&cnx, "/qux/", ev4, (void*)4);

	wc_datasync_unroute_data(&cnx, "/qux/", NULL);

	push.type = WC_PUSH_DATA_UPDATE_PUT;
	push.u.update_put.path = "/foo/bar/baz/qux/";
	push.u.update_put.data = "{\"aaa\": \"bbb\", \"answer\": 42}";

	wc_datasync_dispatch_data(&cnx, &push);

	STFU_TRUE("Callback was called for path /foo/", c & 0x1);
	STFU_TRUE("Callback was called for path /foo/bar/", c & 0x2);
	STFU_TRUE("Callback was called for path /foo/bar/baz/", c & 0x4);
	STFU_TRUE("Callback was never called for path /qux/", !(c & 0x8));

	wc_datasync_route_data(&cnx, "/foo/bar/baz/qux/", ev5, (void*)5);
	wc_datasync_route_data(&cnx, "/", ev6, (void*)6);

	wc_datasync_dispatch_data(&cnx, &push);

	STFU_TRUE("Callback was called for path /foo/bar/baz/qux/", c & 0x10);
	STFU_TRUE("Callback was called for path /", c & 0x20);

	STFU_SUMMARY();

	return STFU_NUMBER_FAILED;
}
