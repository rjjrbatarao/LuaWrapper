/* Lua CJSON floating point conversion routines */

/* Buffer required to store the largest string representation of a double.
 *
 * Longest double printed with %.14g is 21 characters long:
 * -1.7976931348623e+308 */
#ifndef __FP_CONV_
#define __FP_CONV_
# define FPCONV_G_FMT_BUFSIZE   32

#ifdef USE_INTERNAL_FPCONV
static inline void fpconv_init()
{
    /* Do nothing - not required */
}
#else
extern void fpconv_init();
#endif

extern int fpconv_g_fmt(char*, double, int);
extern double fpconv_strtod(const char*, char**);

/* vi:ai et sw=4 ts=4:
 */
#endif