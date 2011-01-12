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

#ifndef PHP_PWEE_H
#define PHP_PWEE_H

extern zend_module_entry pwee_module_entry;
#define phpext_pwee_ptr &pwee_module_entry

#ifdef PHP_WIN32
#define PHP_PWEE_API __declspec(dllexport)
#else
#define PHP_PWEE_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

#include "pwee_string.h"
#include "pwee_if.h"
#include "pwee_conf.h"
#include "pwee_uuid.h"

PHP_MINIT_FUNCTION(pwee);
PHP_MSHUTDOWN_FUNCTION(pwee);
PHP_RINIT_FUNCTION(pwee);
PHP_RSHUTDOWN_FUNCTION(pwee);
PHP_MINFO_FUNCTION(pwee);

PHP_FUNCTION(pwee_info);
PHP_FUNCTION(pwee_getifaddress);
PHP_FUNCTION(pwee_listinterfaces);

ZEND_BEGIN_MODULE_GLOBALS(pwee)
	ifcache* g_pIfCache;					// Network interface information cache
	confEnvironment* g_pSysEnv;				// System environment
	confEnvironment* g_pActiveUserEnv;		// Active user environment - points into g_htUserEnv
	confEnvironment* g_pLastUserEnv;		// Last user environment - points into g_htUserEnv
	int g_bRegisterNetConstants;			// Do we register network related constants?
	int g_bRegisterUIDConstants;			// Do we register UID constants?
	int g_bExposeEnv;						// Do we expose our environment in phpinfo()?
	char* g_pszNetConstantPrefix;			// Prefix for network constants
	char* g_pszSysConfPath;					// Path to system XML environment definition
	char* g_pszUserConfPath;				// Path to user XML environment definition
	unsigned long g_nRequests;				// Number of requests for executor - has potential to roll over
	confValue* g_pRequestUIDValue;			// Request unique identifier value - points into g_pSysEnv
	char g_szRequestUIDPrefix[UUID_LEN+2];	// Prefix for request UID (includes trailing '-')
	int g_bFirstRequest;					// Is this our first request?
	HashTable g_htUserEnv;					// Hash table of user environments (confpath => confEnvironment*)
	HashTable g_htSaveVariables;			// Indexed hash table of variables to save (confValue*)
	int g_bAllowUserEnv;					// Do we allow users to create environments?
ZEND_END_MODULE_GLOBALS(pwee)

#ifdef ZTS
#define PWEE_G(v) TSRMG(pwee_globals_id, zend_pwee_globals *, v)
#else
#define PWEE_G(v) (pwee_globals.v)
#endif

#endif	/* PHP_PWEE_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 */
