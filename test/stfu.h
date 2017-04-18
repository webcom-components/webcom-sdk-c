/*
 * Simple Test Framework Uncomplicated
 */

#ifndef TEST_STFU_H_
#define TEST_STFU_H_

#include <stdio.h>
#include <string.h>

#define STFU_INIT() unsigned int npass = 0, nfail = 0;

#define STFU_TRUE(message, test) \
	do { \
		printf("[....] Testing \"%s\"", (message)); \
		if ((test)) { \
			npass++; printf("\r[" "\033[32m" "PASS" "\033[0m" "\n"); \
		} else { \
			nfail++; printf("\r[" "\033[31m" "FAIL" "\033[0m" "\n\t%s\n", #test); \
		} \
	} while (0)

#define STFU_FALSE(message, test) STFU_TRUE(message, !(test))
#define STFU_STR_EQ(message, str1, str2) STFU_TRUE(message, (strcmp((str1), (str2)) == 0))

#define STFU_SUMMARY() \
	do { \
		printf("\n" "\033[1m" "TOTAL: %u tests, %u passed, %u failed" "\033[0m" "\n", npass + nfail, npass, nfail); \
	} while(0)

#define STFU_NUMBER_FAILED (nfail)

#endif /* TEST_STFU_H_ */
