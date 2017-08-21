#include "../lib/event.c"

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

void ev1(wc_cnx_t *cnx, ws_on_data_event_t event, char *path, char *json_data, void *param) {
	printf("\tcallback 1: [%p:%d:%p] %s => %s\n", cnx, event, param, path, json_data);
	if (param == (void*)0x10101010) {
		c |= 0x1;
	}
}

void ev2(wc_cnx_t *cnx, ws_on_data_event_t event, char *path, char *json_data, void *param) {
	printf("\tcallback 2: [%p:%d:%p] %s => %s\n", cnx, event, param, path, json_data);
	if (param == (void*)0x20202020) {
		c |= 0x2;
	}
}

void ev3(wc_cnx_t *cnx, ws_on_data_event_t event, char *path, char *json_data, void *param) {
	printf("\tcallback 3: [%p:%d:%p] %s => %s\n", cnx, event, param, path, json_data);
	if (param == (void*)0x30303030) {
		c |= 0x4;
	}
}

int main(void)
{
	wc_cnx_t cnx;
	wc_push_t push;
	memset(&cnx, 0, sizeof(cnx));

	STFU_TRUE("Webcom path hash is djb2 hash", path_hash("/foo/bar/bazqux/") == djb2("/foo/bar/bazqux/"));
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

	wc_on_data(&cnx, "/foo/", ev1, (void*)0x10101010);
	wc_on_data(&cnx, "/foo/bar/", ev2, (void*)0x20202020);
	wc_on_data(&cnx, "/foo/bar/baz/", ev3, (void*)0x30303030);

	push.type = WC_PUSH_DATA_UPDATE_PUT;
	push.u.update_put.path = "/foo/bar/baz/qux/";
	push.u.update_put.data = "{\"aaa\": \"bbb\", \"answer\": 42}";

	wc_on_data_dispatch(&cnx, &push);

	STFU_TRUE("Callback was called for path /foo/", c & 0x1);
	STFU_TRUE("Callback was called for path /foo/bar/", c & 0x2);
	STFU_TRUE("Callback was called for path /foo/bar/baz/", c & 0x4);

	STFU_SUMMARY();

	return STFU_NUMBER_FAILED;
}
