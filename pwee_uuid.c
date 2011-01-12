/*
	Copyright (c) 2002, Lance Lovette
	All rights reserved.

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to
	deal in the Software without restriction, including without limitation the
	rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in
	all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
	IN THE SOFTWARE.
*/

#include "php.h"

#include "php_pwee.h"

#ifdef HAVE_LIBUUID
#include <uuid/uuid.h>
#endif

/* {{{ int ulongtobase36(unsigned long value, char* out, int outlen)
   Converts a long to a string containing a base36 representation of the number.
   out should be able to store at least 8 characters (includes the terminating `\0' character).
   Returns out if successful or NULL if the buffer is too small. 
   The largest number you can get out of this function is 4,294,967,295 = 1z141z3 */
char* ulongtobase36(unsigned long value, char* out, int outlen)
{
	static char digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";

	if (outlen < 2)
	{
		if (outlen > 0)
			*out = '\0';
		return NULL;
	}

	if (value > 0)
	{
		int numdigits = 0;
		char* ptr = out + (outlen - 1);

		for (*ptr-- = '\0'; (value > 0) && (ptr >= out); value /= 36, numdigits++)
		{
			*ptr-- = digits[value % 36];
		}

		ptr++;

		// If there is still a value the buffer is too small
		if (value > 0)
		{
			*out = '\0';
			return NULL;
		}

		memmove(out, ptr, numdigits+1);
	}
	else
	{
		char* ptr = out;
		*ptr++ = digits[0];
		*ptr = '\0';
	}

	return out;
}	
/* }}} */

/* {{{ char* pwee_uuid_generate(char*)
   out must be able to hold UUID_LEN characters plus the terminating `\0' character. */
char* pwee_uuid_generate(char* out)
{
#ifdef HAVE_LIBUUID
   uuid_t uu;
   uuid_generate(uu);
   uuid_unparse(uu, out);
#else
   PHP_STRLCPY(out, "TODO", UUID_LEN+1, strlen("TODO"));
#endif

   return out;
}
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 */
