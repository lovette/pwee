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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"

#include "php_pwee.h"

int incrementExecutorRequestUID(confValue* pValue);
int addNetConstantsToEnvironment(confEnvironment*);
int registerEnvironmentConstants(confEnvironment* pEnv, int module_number);
int registerEnvironmentVariables(confEnvironment* pEnv, int module_number);
int unregisterEnvironmentConstants(confEnvironment* pEnv);
int addExecutorConstantsToEnvironment(confEnvironment* pEnv, int module_number);
int saveEnvironmentVariables(HashTable* ht);
void printEnvironmentInfo(confEnvironment* pEnv);
int activateUserEnvironment(pweeString* pstrFilename);

#define PHP_PWEE_EXTNAME "pwee"
#define PHP_PWEE_EXTVER  "1.2"

#define UUID_SUFFIX_LEN	4		// Largest four character value in base 36 is zzzz
#define BASE36_ZZZZ		1679616 // Largest four character value in base 36 is 1679615
#define HT_SYMBOL_TABLE	&EG(symbol_table)
#define HT_CONSTANTS	EG(zend_constants)
#define debug_out		php_error
#define CONSTANT_FLAGS	(CONST_CS | CONST_PERSISTENT)

// The standard REGISTER_*_CONSTANT macros assume storage for the name and value are statically allocated.
// To this end, Zend only estrdups the constant name and not the value. Note that MYREGISTER_STRING_CONSTANT
// registers a pointer to the actual string value. Do not change the storage without updating the constant
// or Zend will be pointing to an invalid memory location.
#define MYREGISTER_LONG_CONSTANT(name, lval)			zend_register_long_constant(PSTR_STRVAL(name), PSTR_STRLEN(name)+1, (lval), (CONSTANT_FLAGS), module_number TSRMLS_CC)
#define MYREGISTER_BOOL_CONSTANT(name, lval)			register_bool_constant(PSTR_STRVAL(name), PSTR_STRLEN(name)+1, (lval), (CONSTANT_FLAGS), module_number TSRMLS_CC)
#define MYREGISTER_DOUBLE_CONSTANT(name, dval)			zend_register_double_constant(PSTR_STRVAL(name), PSTR_STRLEN(name)+1, (dval), (CONSTANT_FLAGS), module_number TSRMLS_CC)
#define MYREGISTER_STRING_CONSTANT(name, sval, slen)	zend_register_stringl_constant(PSTR_STRVAL(name), PSTR_STRLEN(name)+1, (sval), (slen), (CONSTANT_FLAGS), module_number TSRMLS_CC)

#define UNREGISTER_CONSTANT(name) unregister_constant(&(name))
#define UNREGISTER_GLOBAL_VAR(name) zend_hash_del(HT_SYMBOL_TABLE, PSTR_STRVAL(name), PSTR_STRLEN(name)+1)
#define MY_SET_GLOBAL_VAR(name, pzval) ZEND_SET_GLOBAL_VAR_WITH_LENGTH(PSTR_STRVAL(name), PSTR_STRLEN(name)+1, pzval, 1, 0);

//////////////////////////////////////////////////////////////////////////
//
// Module setup
//
//////////////////////////////////////////////////////////////////////////

ZEND_DECLARE_MODULE_GLOBALS(pwee)

/* {{{ pwee_functions[]
 *
 */
static zend_function_entry pwee_functions[] = {
	PHP_FE(pwee_info,	        NULL)
//	PHP_FE(pwee_getifaddress,	NULL)
//	PHP_FE(pwee_listinterfaces,	NULL)
	{NULL, NULL, NULL}
};
/* }}} */

/* {{{ php_pwee_deps[]
  * As of PHP 5.1, interdependencies between extensions can be enforced
  */
#if ZEND_MODULE_API_NO >= 20050617
static zend_module_dep pwee_deps[] = {
	ZEND_MOD_REQUIRED("libxml")
	ZEND_MOD_END
};
#endif
/* }}} */

/* {{{ pwee_module_entry
 */
zend_module_entry pwee_module_entry = {
#if ZEND_MODULE_API_NO >= 20050617
	STANDARD_MODULE_HEADER_EX,
	NULL,
	pwee_deps,
#else
	STANDARD_MODULE_HEADER,
#endif
	PHP_PWEE_EXTNAME,
	pwee_functions,
	PHP_MINIT(pwee),
	PHP_MSHUTDOWN(pwee),
	PHP_RINIT(pwee),
	PHP_RSHUTDOWN(pwee),
	PHP_MINFO(pwee),
    PHP_PWEE_EXTVER,
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_PWEE
ZEND_GET_MODULE(pwee)
#endif

//////////////////////////////////////////////////////////////////////////
//
// INI
//
//////////////////////////////////////////////////////////////////////////

/* {{{ OnUpdateUserConfPath
   If there is a userconfpath setting in effect, this function will be
   called before and after each request. We need to setup and tear down
   the active user environment based on the userconfpath. */
PHP_INI_MH(OnUpdateUserConfPath)
{
	if (PWEE_G(g_bAllowUserEnv) && (NULL != new_value) && (*new_value))
	{
		pweeString strFilename;
		PSTR_SETVALUEL(strFilename, new_value, new_value_length, 1); // callee will free
		return activateUserEnvironment(&strFilename);
	}
	else
	{
		PWEE_G(g_pActiveUserEnv) = NULL;
	}

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_INI
 */
PHP_INI_BEGIN()
    STD_ZEND_INI_ENTRY  ("pwee.sysconfpath",			"",			PHP_INI_SYSTEM, OnUpdateString,			g_pszSysConfPath,			zend_pwee_globals, pwee_globals)
    STD_ZEND_INI_ENTRY  ("pwee.userconfpath",			"",			PHP_INI_ALL,	OnUpdateUserConfPath,	g_pszUserConfPath,			zend_pwee_globals, pwee_globals)
    STD_ZEND_INI_BOOLEAN("pwee.exposeinfo",				"1",		PHP_INI_ALL,	OnUpdateBool,			g_bExposeEnv,				zend_pwee_globals, pwee_globals)
    STD_ZEND_INI_BOOLEAN("pwee.userconf_allow",			"1",		PHP_INI_SYSTEM,	OnUpdateBool,			g_bAllowUserEnv,			zend_pwee_globals, pwee_globals)
    STD_ZEND_INI_ENTRY  ("pwee.net_constant_prefix",	"SERVER",	PHP_INI_SYSTEM,	OnUpdateString,			g_pszNetConstantPrefix,		zend_pwee_globals, pwee_globals)
    STD_ZEND_INI_BOOLEAN("pwee.register_net_constants",	"1",		PHP_INI_SYSTEM,	OnUpdateBool,			g_bRegisterNetConstants,	zend_pwee_globals, pwee_globals)
    STD_ZEND_INI_BOOLEAN("pwee.register_uid_constants",	"1",		PHP_INI_SYSTEM,	OnUpdateBool,			g_bRegisterUIDConstants,	zend_pwee_globals, pwee_globals)
PHP_INI_END()
/* }}} */

//////////////////////////////////////////////////////////////////////////
//
// Module globals functions
//
//////////////////////////////////////////////////////////////////////////

/* {{{ htUserEnv_dtor
   Destroy elements in the g_htUserEnv hash table. */
static void htUserEnv_dtor(void* p)
{
	confEnvironment** ppEnv = p;
	confEnvironment_dtor(*ppEnv);
	free(*ppEnv);
	 // do not set *ppEnv to NULL, trust me
}
/* }}} */

/* {{{ php_pwee_globals_ctor
 */
static void php_pwee_globals_ctor(zend_pwee_globals *pwee_globals)
{
	pwee_globals->g_bRegisterNetConstants = 1;
	pwee_globals->g_bRegisterUIDConstants = 1;
	pwee_globals->g_bExposeEnv = 1;
	pwee_globals->g_bAllowUserEnv = 1;
	pwee_globals->g_pszNetConstantPrefix = NULL;
	pwee_globals->g_pszSysConfPath = NULL;
	pwee_globals->g_pIfCache = ifcache_new();
	pwee_globals->g_pSysEnv = confEnvironment_new();
	pwee_globals->g_pActiveUserEnv = NULL;
	pwee_globals->g_pLastUserEnv = NULL;
	pwee_globals->g_pRequestUIDValue = NULL;
	pwee_globals->g_nRequests = 0;
	pwee_globals->g_szRequestUIDPrefix[0] = 0;
	pwee_globals->g_bFirstRequest = 1;

	zend_hash_init(&pwee_globals->g_htUserEnv, 0, NULL, htUserEnv_dtor, 1);
	zend_hash_init(&pwee_globals->g_htSaveVariables, 0, NULL, NULL, 1);
}
/* }}} */

/* {{{ php_pwee_globals_dtor
 */
static void php_pwee_globals_dtor(zend_pwee_globals *pwee_globals)
{
	ifcache_delete(&pwee_globals->g_pIfCache);
	confEnvironment_delete(&pwee_globals->g_pSysEnv);
	zend_hash_destroy(&pwee_globals->g_htUserEnv);
	zend_hash_destroy(&pwee_globals->g_htSaveVariables);
}
/* }}} */

//////////////////////////////////////////////////////////////////////////
//
// Private helpers
//
//////////////////////////////////////////////////////////////////////////

/* {{{ register_bool_constant
   Zend doesn't have a function to register boolean constants (at this time anyway) */
static void register_bool_constant(char *name, uint name_len, long lval, int flags, int module_number TSRMLS_DC)
{
	zend_constant c;

	c.value.type = IS_BOOL;
	c.value.value.lval = lval;
	c.flags = flags;
	c.name = zend_strndup(name, name_len);
	c.name_len = name_len;
	c.module_number = module_number;

	zend_register_constant(&c TSRMLS_CC);
}
/* }}} */

/* {{{ overwrite_string_constant_mem
   This function overwrites the memory for a string constant value directly.
   You better know what you're working with because it doesn't care about
   overstepping its bounds. The the terminating `\0' character is always
   included in the overwrite. */
static int overwrite_string_constant_mem(const pweeString* pstrName, char* strval, int slen TSRMLS_DC)
{
	int retval = FAILURE;
	zend_constant* pc = NULL;

	// All of our constants are CONST_CS so we just skip that check
	if (SUCCESS == zend_hash_find(HT_CONSTANTS, PSTR_STRVAL_P(pstrName), PSTR_STRLEN_P(pstrName)+1, (void **) &pc))
	{
		pc->value.value.str.len = slen;
		memcpy(pc->value.value.str.val, strval, pc->value.value.str.len+1);
		retval = SUCCESS;
	}

	return retval;
}
/* }}} */

/* {{{ unregister_constant
   Zend doesn't have a function to unregister constants (at this time anyway) */
static int unregister_constant(const pweeString* pstrName)
{
	int retval = FAILURE;
	zend_constant* pc = NULL;

	// All of our constants are CONST_CS so we just skip that check
	if (SUCCESS == zend_hash_find(HT_CONSTANTS, PSTR_STRVAL_P(pstrName), PSTR_STRLEN_P(pstrName)+1, (void **) &pc))
	{
		retval = zend_hash_del(HT_CONSTANTS, PSTR_STRVAL_P(pstrName), PSTR_STRLEN_P(pstrName)+1);
	}

	return retval;
}
/* }}} */

//////////////////////////////////////////////////////////////////////////
//
// Startup/shutdown/phpinfo functions
//
//////////////////////////////////////////////////////////////////////////

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(pwee)
{
	ZEND_INIT_MODULE_GLOBALS(pwee, php_pwee_globals_ctor, php_pwee_globals_dtor);

	REGISTER_INI_ENTRIES();

	REGISTER_STRING_CONSTANT("PWEE_VERSION", PHP_PWEE_EXTVER, CONST_CS | CONST_PERSISTENT);

	// Cache the list of active interfaces
	ifcache_enuminterfaces(PWEE_G(g_pIfCache));

	// Add network interface constants to the environment
	if (PWEE_G(g_bRegisterNetConstants))
	{
		if (SUCCESS != addNetConstantsToEnvironment(PWEE_G(g_pSysEnv)))
			return FAILURE;
	}

	// Add external constants and variables to the environment
	if (PWEE_G(g_pszSysConfPath) && *PWEE_G(g_pszSysConfPath))
	{
		if (SUCCESS != confEnvironment_parseFile(PWEE_G(g_pSysEnv), PWEE_G(g_pszSysConfPath), NULL))
			return FAILURE;
	}

	// Register environment constants
	if (confEnvironment_hasAnyConstants(PWEE_G(g_pSysEnv)))
	{
		registerEnvironmentConstants(PWEE_G(g_pSysEnv), module_number);
	}

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(pwee)
{
	UNREGISTER_INI_ENTRIES();

	php_pwee_globals_dtor(&pwee_globals);

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(pwee)
{
	PWEE_G(g_nRequests)++;

	if (PWEE_G(g_bFirstRequest))
	{
		addExecutorConstantsToEnvironment(PWEE_G(g_pSysEnv), module_number);
		PWEE_G(g_bFirstRequest) = 0;
	}

	if (zend_hash_num_elements(&PWEE_G(g_htSaveVariables)))
	{
		php_error(E_ERROR, "Why are there variables in the saved variable cache?");
	}

	incrementExecutorRequestUID(PWEE_G(g_pRequestUIDValue));

	// Tear down the last user environment if it's no longer active.
	// Zend resets the symbol_table before each request so all we need
	// to do is register variables. Constants however are persistent
	// so we need to remove the old ones before adding new ones.
	if (PWEE_G(g_pLastUserEnv) != PWEE_G(g_pActiveUserEnv))
	{
		unregisterEnvironmentConstants(PWEE_G(g_pLastUserEnv));

		registerEnvironmentConstants(PWEE_G(g_pActiveUserEnv), module_number);

		PWEE_G(g_pLastUserEnv) = PWEE_G(g_pActiveUserEnv);
	}

	// Environment variables have to be registered on each request
	registerEnvironmentVariables(PWEE_G(g_pSysEnv), module_number);
	registerEnvironmentVariables(PWEE_G(g_pActiveUserEnv), module_number);

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(pwee)
{
	HashTable* htSaveVariables = &PWEE_G(g_htSaveVariables);

	// Save the values for environment variables with 'executor' scope.
	if (zend_hash_num_elements(htSaveVariables))
	{
		saveEnvironmentVariables(htSaveVariables);
		zend_hash_clean(htSaveVariables);
	}

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(pwee)
{
	char sz[32];

	php_info_print_table_start();
	php_info_print_table_row(2, "Pwee support", "enabled");
	php_info_print_table_row(2, "Extension version", PHP_PWEE_EXTVER);

	if (PWEE_G(g_bExposeEnv))
	{
		sprintf(sz, "%d", getpid());
		php_info_print_table_row(2, "Executor PID", sz);
		sprintf(sz, "%lu", PWEE_G(g_nRequests));
		php_info_print_table_row(2, "Executor request count", sz);
	}

	php_info_print_table_end();

	DISPLAY_INI_ENTRIES();

	printEnvironmentInfo(PWEE_G(g_pSysEnv));
	printEnvironmentInfo(PWEE_G(g_pActiveUserEnv));
}
/* }}} */

//////////////////////////////////////////////////////////////////////////
//
// Public functions
//
//////////////////////////////////////////////////////////////////////////

/* {{{ proto void pwee_info()
   Outputs the same module information as phpinfo would. */
PHP_FUNCTION(pwee_info)
{
	zm_info_pwee(&pwee_module_entry);
	RETURN_NULL();
}
/* }}} */

/* {{{ proto string pwee_getifaddress(string ifname)
   Returns the IP address of an active network interface. ifname is the name
   of the interface. This is usually a driver name followed by a unit number, for
   example eth0 for the first Ethernet interface. */
PHP_FUNCTION(pwee_getifaddress)
{
	zval **z_ifname;
	char* ifaddr = NULL;
	char myaddress[IFINFONAME+1];

	if ((ZEND_NUM_ARGS() != 1) || (zend_parse_parameters(1 TSRMLS_CC, "z", &z_ifname) != SUCCESS))
	{
		WRONG_PARAM_COUNT;
	}

	convert_to_string_ex(z_ifname);

	ifaddr = getifaddress(Z_STRVAL_PP(z_ifname), myaddress, sizeof(myaddress));
	if (NULL == ifaddr)
	{
		zend_error(E_WARNING, "Failed to get IP address for interface '%s'", Z_STRVAL_PP(z_ifname));
		RETURN_FALSE;
	}

	RETURN_STRING(ifaddr, 1);
}
/* }}} */

/* {{{ proto array pwee_listinterfaces([refresh])
   Return an associative array containing the IP address of each active network interface. */
PHP_FUNCTION(pwee_listinterfaces)
{
	int i = 0;
	int refreshcache = 0; // default is NO
	ifcache* pifcache = PWEE_G(g_pIfCache);
	zval **z_refreshcache = NULL;

	if (ZEND_NUM_ARGS() == 1)
	{
		if (zend_parse_parameters(1 TSRMLS_CC, "z", &z_refreshcache) != SUCCESS)
			WRONG_PARAM_COUNT;

		convert_to_long_ex(z_refreshcache);

		refreshcache = Z_LVAL_PP(z_refreshcache);
	}

	// The user can decide if they want to use the cached
	// interface information or update the cache and use new information.
	if (refreshcache)
	{
		ifcache_dtor(pifcache);
		ifcache_enuminterfaces(pifcache);
	}

	if (NULL == pifcache->iflist)
	{
		php_error(E_WARNING, "It appears this server has no active network interfaces");
		RETURN_FALSE;
	}

	if (array_init(return_value) != SUCCESS)
	{
		php_error(E_ERROR, "Failed to initialize the return array");
		RETURN_FALSE;
	}

	for (i=0; i < pifcache->ifcount; i++)
	{
		add_assoc_string(return_value, pifcache->iflist[i].szifname, pifcache->iflist[i].szifaddr, 1);
	}

	// return_value is the return value...
}
/* }}} */

//////////////////////////////////////////////////////////////////////////
//
// Internal helpers
//
//////////////////////////////////////////////////////////////////////////

/* {{{ int printEnvironmentInfo(confEnvironment* pEnv)
   Prints out environment information. */
void printEnvironmentInfo(confEnvironment* pEnv)
{
	if ((NULL != pEnv) && PWEE_G(g_bExposeEnv))
	{
		if (confEnvironment_hasAnyVariables(pEnv) || confEnvironment_hasAnyConstants(pEnv))
		{
			confApplication* pApp = NULL;

			for (pApp=pEnv->pApps; NULL != pApp; pApp=pApp->pNext)
			{
				confValue* pValue = NULL;

				php_info_print_table_start();

				php_info_print_table_colspan_header(3, PSTR_STRLEN(pApp->strAppName) ? PSTR_STRVAL(pApp->strAppName) : "(Unnamed)");
				php_info_print_table_header(3, "Name", "Value", "Scope");

				for (pValue=pApp->pConstants; NULL != pValue; pValue=pValue->pNext)
				{
					php_info_print_table_row(3, PSTR_STRVAL(pValue->strValueName),
						confValue_getValueAsString(pValue),
						"constant");
				}

				for (pValue=pApp->pVariables; NULL != pValue; pValue=pValue->pNext)
				{
					php_info_print_table_row(3, PSTR_STRVAL(pValue->strValueName),
						confValue_getValueAsString(pValue),
						scopeToString(pValue->scope));
				}

				php_info_print_table_end();
			}
		}
	}
}
/* }}} */

/* {{{ int addNetConstantsToEnvironment(confEnvironment* pEnv)
   Adds network related constants to the environment. */
int addNetConstantsToEnvironment(confEnvironment* pEnv)
{
	if (NULL != pEnv)
	{
		int i = 0;
		ifcache* pifcache = PWEE_G(g_pIfCache);
		char szconstant[128];
		int setexternaladdr = 0;
		char myname[MAXHOSTNAMELEN+1];
		pweeString strName;
		pweeString strValue;
		confApplication* pApp = confApplication_new();

		PSTR_SETVALUEL(pApp->strAppName, "NETWORK", sizeof("NETWORK")-1, 1);
		PSTR_SETVALUE(pApp->strNamespace, PWEE_G(g_pszNetConstantPrefix), 1);
		pApp->bUpperCaseConstants = 1;

		// Register constants with the addresses of each interface.
		for (i=0; i < pifcache->ifcount; i++)
		{
			confValue* pValue = confValue_new();
			sprintf(szconstant, "IFADDR_%.16s", pifcache->iflist[i].szifname);
			PSTR_SETVALUE(strName, szconstant, 0);
			PSTR_SETVALUE(strValue, pifcache->iflist[i].szifaddr, 0);
			confValue_setValue(pValue, &strName, &strValue, "string", 1, pApp);
			confApplication_addValue(pApp, pValue);

			// Register a constant with the address of the first interface with an external address.
			if (!setexternaladdr && (0 != strcmp("127.0.0.1", pifcache->iflist[i].szifaddr)))
			{
				confValue* pValue = confValue_new();
				PSTR_SETVALUEL(strName, "IFADDR", sizeof("IFADDR")-1, 0);
				confValue_setValue(pValue, &strName, &strValue, "string", 1, pApp);
				confApplication_addValue(pApp, pValue);
				setexternaladdr = 1;
			}
		}

		// Register the host full name
		if (NULL != gethostnamepart(myname, sizeof(myname), "f"))
		{
			confValue* pValue = confValue_new();
			PSTR_SETVALUEL(strName, "HOSTNAME", sizeof("HOSTNAME")-1, 0);
			PSTR_SETVALUE(strValue, myname, 0);
			confValue_setValue(pValue, &strName, &strValue, "string", 1, pApp);
			confApplication_addValue(pApp, pValue);
		}

		// Register the host short name
		if (NULL != gethostnamepart(myname, sizeof(myname), "s"))
		{
			confValue* pValue = confValue_new();
			PSTR_SETVALUEL(strName, "HOSTSHORTNAME", sizeof("HOSTSHORTNAME")-1, 0);
			PSTR_SETVALUE(strValue, myname, 0);
			confValue_setValue(pValue, &strName, &strValue, "string", 1, pApp);
			confApplication_addValue(pApp, pValue);
		}

		// Register the host domain name
		if (NULL != gethostnamepart(myname, sizeof(myname), "d"))
		{
			confValue* pValue = confValue_new();
			PSTR_SETVALUEL(strName, "HOSTDOMAIN", sizeof("HOSTDOMAIN")-1, 0);
			PSTR_SETVALUE(strValue, myname, 0);
			confValue_setValue(pValue, &strName, &strValue, "string", 1, pApp);
			confApplication_addValue(pApp, pValue);
		}

		confEnvironment_addApplication(pEnv, pApp, 1);
	}

	return SUCCESS;
}
/* }}} */

/* {{{ int registerEnvironmentConstants(confEnvironment* pEnv, int module_number)
   Register environment constants. */
int registerEnvironmentConstants(confEnvironment* pEnv, int module_number)
{
	if ((NULL != pEnv) && confEnvironment_hasAnyConstants(pEnv))
	{
		confValue* pValue = NULL;
		confApplication* pApp = NULL;

		for (pApp=pEnv->pApps; NULL != pApp; pApp=pApp->pNext)
		{
			for (pValue=pApp->pConstants; NULL != pValue; pValue=pValue->pNext)
			{
				switch (pValue->type)
				{
				case IS_STRING:
					MYREGISTER_STRING_CONSTANT(pValue->strValueName, pValue->value.str.val, pValue->value.str.len);
					break;
				case IS_LONG:
					MYREGISTER_LONG_CONSTANT(pValue->strValueName, pValue->value.lval);
					break;
				case IS_BOOL:
					MYREGISTER_BOOL_CONSTANT(pValue->strValueName, pValue->value.lval);
					break;
				case IS_DOUBLE:
					MYREGISTER_DOUBLE_CONSTANT(pValue->strValueName, pValue->value.dval);
					break;
				default:
					php_error(E_ERROR, "Undefined environment variable value type");
					break;
				}
			}
		}
	}

	return SUCCESS;
}
/* }}} */

/* {{{ int registerEnvironmentVariables(confEnvironment* pEnv, module_number)
   Register environment variables. */
int registerEnvironmentVariables(confEnvironment* pEnv, int module_number)
{
	if ((NULL != pEnv) && confEnvironment_hasAnyVariables(pEnv))
	{
		zval* pz = NULL;
		confValue* pValue = NULL;
		confApplication* pApp = NULL;

		// Register environment variables
		for (pApp=pEnv->pApps; NULL != pApp; pApp=pApp->pNext)
		{
			for (pValue=pApp->pVariables; NULL != pValue; pValue=pValue->pNext)
			{
				MAKE_STD_ZVAL(pz);

				switch (pValue->type)
				{
				case IS_STRING:
					ZVAL_STRINGL(pz, pValue->value.str.val, pValue->value.str.len, 1);
					break;
				case IS_LONG:
					ZVAL_LONG(pz, pValue->value.lval);
					break;
				case IS_BOOL:
					ZVAL_BOOL(pz, pValue->value.lval);
					break;
				case IS_DOUBLE:
					ZVAL_DOUBLE(pz, pValue->value.dval);
					break;
				default:
					php_error(E_ERROR, "Undefined environment variable value type");
					break;
				}

				// If the variable has executor scope then remember to save its value later
				if (pValue->scope == PWEE_VALUESCOPE_EXECUTOR)
				{
					if (SUCCESS != zend_hash_next_index_insert(&PWEE_G(g_htSaveVariables),
						&pValue, sizeof(confValue*), NULL))
					{
						php_error(E_ERROR, "Failed to add variable to saved variables cache");
					}
				}

				MY_SET_GLOBAL_VAR(pValue->strValueName, pz);
			}
		}
	}

	return SUCCESS;
}
/* }}} */

/* {{{ int unregisterEnvironmentConstants(confEnvironment* pEnv)
   Unregister environment constants. */
int unregisterEnvironmentConstants(confEnvironment* pEnv)
{
	if ((NULL != pEnv) && confEnvironment_hasAnyConstants(pEnv))
	{
		confValue* pValue = NULL;
		confApplication* pApp = NULL;

		for (pApp=pEnv->pApps; NULL != pApp; pApp=pApp->pNext)
			for (pValue=pApp->pConstants; NULL != pValue; pValue=pValue->pNext)
				if (SUCCESS != UNREGISTER_CONSTANT(pValue->strValueName))
					php_error(E_WARNING, "Failed to remove %s from the constants table", PSTR_STRVAL(pValue->strValueName));
	}

	return SUCCESS;
}
/* }}} */

/* {{{ int unregisterEnvironmentVariables(confEnvironment* pEnv, module_number)
   Unregister environment variables.
int unregisterEnvironmentVariables(confEnvironment* pEnv, int module_number)
{
	if (NULL != pEnv)
	{
		confValue* pValue = NULL;
		confApplication* pApp = NULL;

		for (pApp=pEnv->pApps; NULL != pApp; pApp=pApp->pNext)
			for (pValue=pApp->pVariables; NULL != pValue; pValue=pValue->pNext)
				if (SUCCESS != UNREGISTER_GLOBAL_VAR(pValue->strValueName))
					php_error(E_WARNING, "Failed to remove %s from the symbol table", PSTR_STRVAL(pValue->strValueName));
	}

	return SUCCESS;
}
}}} */

/* {{{ int saveEnvironmentVariables(HashTable* ht)
   Save environment variables. */
int saveEnvironmentVariables(HashTable* ht)
{
	confValue** ppValue = NULL;

	for (zend_hash_internal_pointer_reset(ht);
		 zend_hash_get_current_data(ht, (void **) &ppValue) == SUCCESS;
		 zend_hash_move_forward(ht))
	{
		zval** ppz = NULL;

		if (SUCCESS == zend_hash_find(HT_SYMBOL_TABLE,
			PSTR_STRVAL((*ppValue)->strValueName), PSTR_STRLEN((*ppValue)->strValueName)+1, (void**)&ppz))
		{
			confValue_setValueFromZval(*ppValue, *ppz);
		}
		else
		{
			php_error(E_ERROR, "Failed to save value for %s, it's not in the symbol table", PSTR_STRVAL((*ppValue)->strValueName));
		}
	}

	return SUCCESS;
}
/* }}} */

/* {{{ int addExecutorConstantsToEnvironment(confEnvironment* pEnv, int module_number)
   Registers executor specific constants. This function must be called after the
   module is initialized because with Apache (at least) the module is only initialized
   once before the Apache process starts forking. */
int addExecutorConstantsToEnvironment(confEnvironment* pEnv, int module_number)
{
	if (NULL != pEnv)
	{
		pweeString strName;
		pweeString strValue;
		confApplication* pApp = confApplication_new();

		PSTR_SETVALUEL(pApp->strAppName, "EXECUTOR", sizeof("EXECUTOR")-1, 1);
		PSTR_SETVALUEL(pApp->strNamespace, "EXECUTOR", sizeof("EXECUTOR")-1, 1);
		pApp->bUpperCaseConstants = 1;

		if (PWEE_G(g_bRegisterUIDConstants))
		{
			char szUID[UUID_LEN+UUID_SUFFIX_LEN+2];
			confValue* pValue = NULL;

			pwee_uuid_generate(szUID);

			pValue = confValue_new();
			PSTR_SETVALUEL(strName, "UID", sizeof("UID")-1, 0);
			PSTR_SETVALUE(strValue, szUID, 0);
			confValue_setValue(pValue, &strName, &strValue, "string", 1, pApp);
			confApplication_addValue(pApp, pValue);

			MYREGISTER_STRING_CONSTANT(pValue->strValueName, pValue->value.str.val, pValue->value.str.len);

			// We'll set the REQUEST_UID later, this is just a placeholder
			memset(szUID, '*', sizeof(szUID));
			szUID[sizeof(szUID)-1] = '\0';

			pValue = confValue_new();
			PSTR_SETVALUEL(strName, "REQUEST_UID", sizeof("REQUEST_UID")-1, 0);
			PSTR_SETVALUE(strValue, szUID, 0);
			confValue_setValue(pValue, &strName, &strValue, "string", 1, pApp);
			confApplication_addValue(pApp, pValue);
			PWEE_G(g_pRequestUIDValue) = pValue;

			MYREGISTER_STRING_CONSTANT(pValue->strValueName, pValue->value.str.val, pValue->value.str.len);
		}

		confEnvironment_addApplication(pEnv, pApp, 1);
	}

	return SUCCESS;
}
/* }}} */

/* {{{ int incrementExecutorRequestUID(confValue* pValue)
   Updates the executor REQUEST_UID constant. */
int incrementExecutorRequestUID(confValue* pValue)
{
	if (NULL != pValue)
	{
		char szSuffix[8];
		char szUID[UUID_LEN+UUID_SUFFIX_LEN+2];
		unsigned long nSuffix = (PWEE_G(g_nRequests) % BASE36_ZZZZ);
		char* pszUIDPrefix = PWEE_G(g_szRequestUIDPrefix);
		int nSuffixLen = 0;

		// We only want to tack on UUID_SUFFIX_LEN characters to the prefix
		// so we are limited to a maximum base36 number. When
		// the count rolls over we just generate a new prefix.
		if (!(*pszUIDPrefix) || (nSuffix == 0))
		{
			pwee_uuid_generate(pszUIDPrefix);
			strcat(pszUIDPrefix, "-");
		}

		ulongtobase36(nSuffix, szSuffix, sizeof(szSuffix));

		nSuffixLen = strlen(szSuffix);

		// We want to tack on a four digit number that is
		// zero padded on the left.
		// ********-****-****-****-************-0000
		memcpy(szUID, pszUIDPrefix, UUID_LEN + 1);
		memcpy(szUID + UUID_LEN + 1, "0000", UUID_SUFFIX_LEN - nSuffixLen);
		memcpy(szUID + UUID_LEN + 1 + UUID_SUFFIX_LEN - nSuffixLen, szSuffix, nSuffixLen+1);

		// Notice that we are taking advantage of the fact that
		// Zend just keeps a pointer to the original value we allocated
		// for the constant. Overwriting the constant value directly
		// updates the values for both Zend and our internal structure.
		if (SUCCESS != overwrite_string_constant_mem(&(pValue->strValueName), szUID, strlen(szUID)))
			php_error(E_ERROR, "incrementExecutorRequestUID(): Failed to increment %s", PSTR_STRVAL(pValue->strValueName));
	}

	return SUCCESS;
}
/* }}} */

/* {{{ activateUserEnvironment
   Setup or tear down the active user environment. */
int activateUserEnvironment(pweeString* pstrFilename)
{
	int retval = SUCCESS;
	confEnvironment* pEnv = NULL;

	if (pstrFilename && PSTR_STRVAL_P(pstrFilename) && PSTR_STRLEN_P(pstrFilename))
	{
		int bAddEnvToHash = 0;
		char* pszSerial = NULL;
		confEnvironment** ppEnv = NULL;

		// Strip the serial tag from the end of the path
		pszSerial = strrchr(PSTR_STRVAL_P(pstrFilename), ':');
		if (NULL != pszSerial)
		{
			*pszSerial = '\0';
			pszSerial++;
		}

		if (SUCCESS == zend_hash_find(&PWEE_G(g_htUserEnv), PSTR_STRVAL_P(pstrFilename), PSTR_STRLEN_P(pstrFilename)+1, (void**)&ppEnv))
		{
			pEnv = *ppEnv;

			// Check the serial tag and see if we need to reload the configuration
			if ((NULL != pszSerial) && (0 != strcmp(pszSerial, pEnv->pszSerial)))
			{
				// If this environment was the last active environment we need
				// to remove Zend's references to it before it gets destroyed.
				if (pEnv == PWEE_G(g_pLastUserEnv))
				{
					unregisterEnvironmentConstants(pEnv);
					PWEE_G(g_pLastUserEnv) = NULL;
				}

				if (SUCCESS != zend_hash_del(&PWEE_G(g_htUserEnv), PSTR_STRVAL_P(pstrFilename), PSTR_STRLEN_P(pstrFilename)+1))
					php_error(E_ERROR, "Failed to delete environment from user environment cache");

				pEnv = NULL;
				bAddEnvToHash = 1;
			}
		}
		else
		{
			bAddEnvToHash = 1;
		}

		if (bAddEnvToHash)
		{
			pEnv = confEnvironment_new();

			retval = confEnvironment_parseFile(pEnv, PSTR_STRVAL_P(pstrFilename), pszSerial);

			if (SUCCESS == retval)
			{
				retval = zend_hash_add(&PWEE_G(g_htUserEnv), PSTR_STRVAL_P(pstrFilename), PSTR_STRLEN_P(pstrFilename)+1, &pEnv, sizeof(confEnvironment*), NULL);
				if (SUCCESS != retval)
					php_error(E_ERROR, "Failed to add environment to user environment cache");
			}

			if (FAILURE == retval)
			{
				confEnvironment_delete(&pEnv);
				pEnv = NULL;
			}
		}

		PSTR_FREE(*pstrFilename); // caller dup'd the string, we're responsible for it
	}

	PWEE_G(g_pActiveUserEnv) = pEnv;

	return retval;
}
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 tw=78 fdm=marker
 * vim<600: sw=4 ts=4 tw=78
 */
