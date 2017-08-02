#include "../src/webcom.c"

#include "stfu.h"

#define getRandomNumber() (4)

int main(void)
{
	wc_cnx_t cnx;
	unsigned char rnd_dat[12];
	char rnd_str[16];

	memset(&cnx, 0, sizeof(cnx));
	memset(rnd_dat, 0, sizeof(rnd_dat));
	memset(rnd_str, 0, sizeof(rnd_str));

	srand48_r(getRandomNumber(), &cnx.pids.rand_buffer);

	rand_bytes(rnd_dat, 1, &cnx.pids.rand_buffer);
	rand_bytes(rnd_dat, 2, &cnx.pids.rand_buffer);
	rand_bytes(rnd_dat, 3, &cnx.pids.rand_buffer);
	rand_bytes(rnd_dat, 4, &cnx.pids.rand_buffer);
	rand_bytes(rnd_dat, 9, &cnx.pids.rand_buffer);
	rand_bytes(rnd_dat, 10, &cnx.pids.rand_buffer);
	rand_bytes(rnd_dat, 12, &cnx.pids.rand_buffer);

	STFU_TRUE("The PRNG did not segfault (yet)", 1);

	wc_b64ish_encode(rnd_str, rnd_dat, 12);

	printf("\trandom string: %.16s\n", rnd_str);

	STFU_TRUE("We generate a nice-looking 16 chars random string from a 12 bytes random buffer",
		   rnd_str[0] && rnd_str[1] && rnd_str[2] && rnd_str[3]
		&& rnd_str[4] && rnd_str[5] && rnd_str[6] && rnd_str[7]
		&& rnd_str[8] && rnd_str[9] && rnd_str[10] && rnd_str[11]
		&& rnd_str[12] && rnd_str[13] && rnd_str[14] && rnd_str[15]);

	STFU_SUMMARY();

	return STFU_NUMBER_FAILED;
}
