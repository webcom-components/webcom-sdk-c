#ifndef INCLUDE_WEBCOM_C_WEBCOM_UTILS_H_
#define INCLUDE_WEBCOM_C_WEBCOM_UTILS_H_

/**
 * @ingroup webcom-utils
 * @{
 */

/**
 * marks a function parameter unused, and avoids compiler warnings
 * @param expr the parameter definition
 *
 * **Example:**
 *
 * the compiler won't complain about y not being used:
 *
 *			int square(int x, UNUSED_PARAM(int y)) {
 *				return x * x;
 *			}
 */
#define UNUSED_PARAM(expr) expr __attribute__ ((unused))

/**
 * marks a variable unused, and avoids compiler warnings
 * @param name the variable name
 *
 * **Example:**
 *
 * the compiler won't complain about y not being used:
 *
 *			int square(int x) {
 *				int y;
 *				UNUSED_VAR(y);
 *				return x * x;
 *			}
 */
#define UNUSED_VAR(name) (void)(name)

#define WC_INLINE __attribute__((always_inline)) inline

/**
 * @}
 */
#endif /* INCLUDE_WEBCOM_C_WEBCOM_UTILS_H_ */
