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

/*
 * Simple Test Framework Uncomplicated
 */

#ifndef TEST_STFU_H_
#define TEST_STFU_H_

#include <stdio.h>
#include <string.h>

#define STFU_NOINLINE __attribute__ ((noinline))

STFU_NOINLINE static unsigned int _stfu_npass_incr(unsigned int increment) {
	static unsigned int n = 0;
	return (n += increment);
}

STFU_NOINLINE static unsigned int _stfu_nfail_incr(unsigned int increment) {
	static unsigned int n = 0;
	return (n += increment);
}

#define STFU_INFO(fmt, ...) printf("[\033[1;36mINFO\033[0m] " fmt "\n", ## __VA_ARGS__)

#define STFU_TRUE(message, test) \
	do { \
		printf("[....] Testing \"%s\"", (message)); \
		fflush(stdout); \
		if ((test)) { \
			_stfu_npass_incr(1); printf("\r[" "\033[1;32m" "PASS" "\033[0m" "] Tested:\n"); \
		} else { \
			_stfu_nfail_incr(1); printf("\r[" "\033[1;31m" "FAIL" "\033[0m" "] Tested:\n\t%s\n", #test); \
		} \
	} while (0)

#define STFU_FALSE(message, test) STFU_TRUE(message, !(test))
#define STFU_STR_EQ(message, str1, str2) STFU_TRUE(message, (strcmp((str1), (str2)) == 0))

#define STFU_SUMMARY() \
	do { \
		printf("\n" "\033[1m" "TOTAL: %u tests, %u passed, %u failed" "\033[0m" "\n", _stfu_npass_incr(0) + _stfu_nfail_incr(0), _stfu_npass_incr(0), _stfu_nfail_incr(0)); \
	} while(0)

#define STFU_NUMBER_FAILED (_stfu_nfail_incr(0))

#endif /* TEST_STFU_H_ */
