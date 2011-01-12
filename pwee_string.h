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

#ifndef PWEE_STRING_H
#define PWEE_STRING_H

typedef struct _pweeString {
	char* pszData;					// String data
	unsigned int nDataLen;			// String length (excluding null byte)
} pweeString;

#define safe_strndup(s, l) \
	((NULL != (__s)) ? zend_strndup(__s, __l) : __s)

#define PSTR_MIN(a,b) ((a)<(b)?(a):(b))

#define PSTR_STRVAL(v)		(v).pszData
#define PSTR_STRLEN(v)		(v).nDataLen

#define PSTR_STRVAL_P(p)	PSTR_STRVAL(*p)
#define PSTR_STRLEN_P(p)	PSTR_STRLEN(*p)

// Set string value if length is not known; NULL safe
#define PSTR_SETVALUE(v, s, dup) do {								\
	char* __s = (s);												\
	unsigned int __l = (NULL != __s) ? strlen(__s) : 0;				\
	PSTR_STRLEN(v) = __l;											\
	PSTR_STRVAL(v) = (dup ? safe_strndup(__s, __l) : __s);			\
	} while (0)

// Set string value if length is known; not NULL safe
#define PSTR_SETVALUEL(v, s, n, dup) do {							\
	char* __s = (s);												\
	unsigned int __l = n;											\
	PSTR_STRLEN(v) = __l;											\
	PSTR_STRVAL(v) = (dup ? safe_strndup(__s, __l) : __s);			\
	} while (0)

// Frees string data - not string itself!
#define PSTR_FREE(v) \
	_pstr_free(&v);

// Copies char* src into string.
// Sets the string length of the length of src.
#define PSTR_STRLCPY_SZ(dest, src, n) do {							\
	unsigned int __l = n;											\
	PHP_STRLCPY(PSTR_STRVAL(dest), PSTR_STRLEN(dest)+1, src, __l);	\
	PSTR_STRLEN(dest) = PSTR_MIN(__l, PSTR_STRLEN(dest));			\
	} while (0)

// Appends characters from char* src
// THIS IS NOT A STRCAT FUNCTION
// Notice that we don't change the length of the string!
#define	PSTR_APPEND_SZ(dest, src)	\
	strlcat(PSTR_STRVAL(dest), src, PSTR_STRLEN(dest)+1)

// Appends characters from pweeString src
// THIS IS NOT A STRCAT FUNCTION
// Notice that we don't change the length of the string!
#define	PSTR_APPEND_PSTR(dest, src)	\
	PSTR_APPEND_SZ(dest, PSTR_STRVAL(src))

// Don't call directly, use PSTR_FREE, this is inline only
// so we can have the compiler make sure we're freeing the right type
static inline void _pstr_free(pweeString* p)
{
	if (NULL != PSTR_STRVAL_P(p))
	{
		free(PSTR_STRVAL_P(p));
		PSTR_STRVAL_P(p) = NULL;
	}

	PSTR_STRLEN_P(p) = 0;
}

#endif // PWEE_STRING_H

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 */
