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

#define STFU_TRUE(message, test) \
	do { \
		printf("[....] Testing \"%s\"", (message)); \
		if ((test)) { \
			_stfu_npass_incr(1); printf("\r[" "\033[32m" "PASS" "\033[0m" "\n"); \
		} else { \
			_stfu_nfail_incr(1); printf("\r[" "\033[31m" "FAIL" "\033[0m" "\n\t%s\n", #test); \
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
