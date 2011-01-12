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
#include "ext/standard/php_string.h"

#include "php_pwee.h"

#include <libxml/parser.h>

#define SAFE_FREE(p) { if (NULL != p) free(p); p = NULL; }
#define SAFE_XMLFREE(p)	{ if (NULL != p) xmlFree(p); p = NULL; }

//////////////////////////////////////////////////////////////////////////
//
// Internal helpers
//
//////////////////////////////////////////////////////////////////////////

/* {{{ createVarNameWithPrefix
 */
static char* createVarNameWithPrefix(const char* pszName, const char* pszPrefix, const char* pszSeparator)
{
	char* pszVarName = NULL;

	if ((NULL != pszPrefix) && *pszPrefix)
	{
		if ((NULL != pszSeparator) && *pszSeparator)
		{
			pszVarName = malloc(strlen(pszPrefix) + strlen(pszSeparator) + strlen(pszName) + 1);
			sprintf(pszVarName, "%s%s%s", pszPrefix, pszSeparator, pszName);
		}
		else
		{
			pszVarName = malloc(strlen(pszPrefix) + strlen(pszName) + 1);
			sprintf(pszVarName, "%s%s", pszPrefix, pszName);
		}
	}
	else
	{
		pszVarName = strdup(pszName);
	}

	return pszVarName;
}
/* }}} */

//////////////////////////////////////////////////////////////////////////
//
// General helpers
//
//////////////////////////////////////////////////////////////////////////

/* {{{ stringToScope
 */
int stringToScope(const char* scope)
{
	if (NULL == scope)
		return PWEE_VALUESCOPE_REQUEST;
	else if (0 == strcmp(scope, "executor"))
		return PWEE_VALUESCOPE_EXECUTOR;
	return PWEE_VALUESCOPE_REQUEST;
}
/* }}} */

/* {{{ scopeToString
 */
const char* scopeToString(int scope)
{
	switch (scope)
	{
	case PWEE_VALUESCOPE_EXECUTOR:
		return "executor";
		break;
	case PWEE_VALUESCOPE_REQUEST:
	default:
		return "request";
		break;
	}
}
/* }}} */

//////////////////////////////////////////////////////////////////////////
//
// confValue methods
//
//////////////////////////////////////////////////////////////////////////

/* {{{ confValue_new
 */
confValue* confValue_new()
{
	confValue* pThis = malloc(sizeof(confValue));
	confValue_ctor(pThis);
	return pThis;
}
/* }}} */

/* {{{ confValue_delete
 */
void confValue_delete(confValue** ppThis)
{
	if ((NULL != ppThis) && (NULL != *ppThis))
	{
		confValue_dtor(*ppThis);
		free(*ppThis);
		*ppThis = NULL;
	}
}
/* }}} */

/* {{{ confValue_ctor
 */
void confValue_ctor(confValue* pThis)
{
	memset(pThis, 0, sizeof(confValue));
}

/* {{{ confValue_dtor
 */
void confValue_dtor(confValue* pThis)
{
	SAFE_FREE(pThis->pszName);

	if (pThis->type == IS_STRING)
		SAFE_FREE(pThis->value.str.val);

	memset(pThis, 0, sizeof(confValue));
}
/* }}} */

/* {{{ confValue_setValue
 */
int confValue_setValue(confValue* pThis, char* name, char* value, char* type, zend_bool bConstant, confApplication* pApp)
{
	if (NULL == name)
	{
		if (bConstant)
			php_error(E_ERROR, "Constant is not named");
		else
			php_error(E_ERROR, "Variable is not named");
		return 0;
	}

	if (NULL == value)
	{
		if (bConstant)
			php_error(E_ERROR, "Constant has no value");
		else
			php_error(E_ERROR, "Variable has no value");
		return 0;
	}

	pThis->pszName = createVarNameWithPrefix(name, pApp->pszNamespace, "_");

	if (bConstant && pApp->bUpperCaseConstants)
		php_strtoupper(pThis->pszName, strlen(pThis->pszName));

	if ((NULL == type) || (0 == xmlStrcmp(type, "string")))
	{
		pThis->type = IS_STRING;
		pThis->value.str.val = strdup(value);
		pThis->value.str.len = strlen(value);
	}
	else if (0 == xmlStrcmp(type, "boolean"))
	{
		pThis->type = IS_BOOL;
		pThis->value.lval = ((0 == strcmp(value, "true")) || (0 == strcmp(value, "1")) || (0 == strcmp(value, "yes")) || (0 == strcmp(value, "on")));
	}
	else if (0 == xmlStrcmp(type, "long"))
	{
		pThis->type = IS_LONG;
		pThis->value.lval = strtol(value, NULL, 10);
	}
	else if (0 == xmlStrcmp(type, "double"))
	{
		pThis->type = IS_DOUBLE;
		pThis->value.dval = strtod(value, NULL);
	}
	
	if (0 == pThis->type)
	{
		if (bConstant)
			php_error(E_ERROR, "%s: %s is not a valid constant type", name, type);
		else
			php_error(E_ERROR, "%s: %s is not a valid variable type", name, type);
		return 0;
	}

	pThis->bConstant = bConstant;

	return 1;
}
/* }}} */

/* {{{ confValue_setValueFromZval
 */
int confValue_setValueFromZval(confValue* pThis, zval* pz)
{
	if (pz->type != pThis->type)
	{
		php_error(E_ERROR, "confValue_setValueFromZval variable types are not the same");
		return 0;
	}

	switch (pz->type)
	{
	case IS_STRING:
		SAFE_FREE(pThis->value.str.val);
		pThis->value.str.val = strdup(pz->value.str.val);
		pThis->value.str.len = pz->value.str.len;
		break;
	case IS_LONG:
		pThis->value.lval = pz->value.lval;
		break;
	case IS_BOOL:
		pThis->value.lval = pz->value.lval;
		break;
	case IS_DOUBLE:
		pThis->value.dval = pz->value.dval;
		break;
	}

	return 1;
}
/* }}} */

/* {{{ confValue_getValueAsString
 */
char* confValue_getValueAsString(confValue* pThis)
{
	static char sz[32]; // TODO: needs to be TLS

	switch (pThis->type)
	{
	case IS_STRING:
		return pThis->value.str.val;
		break;
	case IS_LONG:
		sprintf(sz, "%ld", pThis->value.lval);
		return sz;
		break;
	case IS_BOOL:
		sprintf(sz, "%s", (pThis->value.lval) ? "true" : "false");
		return sz;
		break;
	case IS_DOUBLE:
		sprintf(sz, "%f", pThis->value.dval);
		return sz;
		break;
	}

	return NULL;
}
/* }}} */

/* {{{ confValue_getTypeAsString
 */
char* confValue_getTypeAsString(confValue* pThis)
{
	switch (pThis->type)
	{
	case IS_STRING:
		return "string";
		break;
	case IS_LONG:
		return "long";
		break;
	case IS_BOOL:
		return "boolean";
		break;
	case IS_DOUBLE:
		return "double";
		break;
	}

	return NULL;
}
/* }}} */

//////////////////////////////////////////////////////////////////////////
//
// confApplication methods
//
//////////////////////////////////////////////////////////////////////////

/* {{{ confApplication_new
 */
confApplication* confApplication_new()
{
	confApplication* pThis = malloc(sizeof(confApplication));
	confApplication_ctor(pThis);
	return pThis;
}
/* }}} */

/* {{{ confApplication_delete
 */
void confApplication_delete(confApplication** ppThis)
{
	if ((NULL != ppThis) && (NULL != *ppThis))
	{
		confApplication_dtor(*ppThis);
		free(*ppThis);
		*ppThis = NULL;
	}
}
/* }}} */

/* {{{ confApplication_ctor
 */
void confApplication_ctor(confApplication* pThis)
{
	memset(pThis, 0, sizeof(confApplication));
}
/* }}} */

/* {{{ confApplication_dtor
 */
void confApplication_dtor(confApplication* pThis)
{
	confValue* pValue = NULL;
	confValue* pNextValue = NULL;

	SAFE_FREE(pThis->pszName);
	SAFE_FREE(pThis->pszNamespace);

	// Free variables
	pValue = pThis->pVariables;
	while (NULL != pValue)
	{
		pNextValue = pValue->pNext;
		confValue_dtor(pValue);
		free(pValue);
		pValue = pNextValue;
	}

	// Free constants
	pValue = pThis->pConstants;
	while (NULL != pValue)
	{
		pNextValue = pValue->pNext;
		confValue_dtor(pValue);
		free(pValue);
		pValue = pNextValue;
	}

	memset(pThis, 0, sizeof(confApplication));
}
/* }}} */

/* {{{ confApplication_addValue
 */
void confApplication_addValue(confApplication* pThis, confValue* pValue)
{
	confValue* pEOL = (pValue->bConstant) ? pThis->pConstants : pThis->pVariables;

	if (NULL == pEOL)
	{
		// This is the first element in the list
		if (pValue->bConstant)
			pThis->pConstants = pValue;
		else
			pThis->pVariables = pValue;
	}
	else
	{
		// Add new element to the end of the list
		while (NULL != pEOL->pNext)
			pEOL = pEOL->pNext;
		pEOL->pNext = pValue;
	}

	pValue->pNext = NULL;
}
/* }}} */

/* {{{ confApplication_hasAnyConstants
 */
int confApplication_hasAnyConstants(confApplication* pThis)
{
	return ((NULL != pThis) && (NULL != pThis->pConstants));
}
/* }}} */

/* {{{ confApplication_hasAnyVariables
 */
int confApplication_hasAnyVariables(confApplication* pThis)
{
	return ((NULL != pThis) && (NULL != pThis->pVariables));
}
/* }}} */

//////////////////////////////////////////////////////////////////////////
//
// XML document parsing functions
//
//////////////////////////////////////////////////////////////////////////

/* {{{ parseConstant
 */
confValue* parseConstant(xmlDocPtr doc, xmlNodePtr cur, confApplication* pApp, char* pszParentPrefix)
{
	char* pszVarName = NULL;
	xmlChar* name = xmlGetProp(cur, "name");
	xmlChar* value = xmlGetProp(cur, "value");
	xmlChar* type = xmlGetProp(cur, "type");
	confValue* pValue = confValue_new();

	pszVarName = createVarNameWithPrefix(name, pszParentPrefix, "_");

	if (!confValue_setValue(pValue, pszVarName, value, type, 1, pApp))
	{
		confValue_delete(&pValue);
		pValue = NULL;
	}

	SAFE_FREE(pszVarName);
	SAFE_XMLFREE(name);
	SAFE_XMLFREE(value);
	SAFE_XMLFREE(type);

	return pValue;
}
/* }}} */

/* {{{ parseVariable
 */
confValue* parseVariable(xmlDocPtr doc, xmlNodePtr cur, confApplication* pApp, const char* pszParentScope, const char* pszParentPrefix)
{
	char* pszVarName = NULL;
	xmlChar* name = xmlGetProp(cur, "name");
	xmlChar* value = xmlGetProp(cur, "value");
	xmlChar* type = xmlGetProp(cur, "type");
	xmlChar* scope = xmlGetProp(cur, "scope");
	confValue* pValue = confValue_new();

	pValue->scope = stringToScope((NULL != scope) ? (const char*)scope : pszParentScope);

	pszVarName = createVarNameWithPrefix(name, pszParentPrefix, "_");

	if (!confValue_setValue(pValue, pszVarName, value, type, 0, pApp))
	{
		confValue_delete(&pValue);
		pValue = NULL;
	}

	SAFE_FREE(pszVarName);
	SAFE_XMLFREE(name);
	SAFE_XMLFREE(value);
	SAFE_XMLFREE(type);
	SAFE_XMLFREE(scope);

	return pValue;
}
/* }}} */

/* {{{ parseConstants
 */
int parseConstants(xmlDocPtr doc, xmlNodePtr cur, confApplication* pApp)
{
	xmlChar* prefix = xmlGetProp(cur, "prefix");

	for (cur=cur->xmlChildrenNode; cur != NULL; cur=cur->next)
	{
		if (0 == xmlStrcmp(cur->name, "Constant"))
		{
			confValue* pValue = parseConstant(doc, cur, pApp, prefix);

			if (NULL == pValue)
				return FAILURE;

			confApplication_addValue(pApp, pValue);
		}
	}

	SAFE_XMLFREE(prefix);

	return SUCCESS;
}
/* }}} */

/* {{{ parseVariables
 */
int parseVariables(xmlDocPtr doc, xmlNodePtr cur, confApplication* pApp)
{
	xmlChar* prefix = xmlGetProp(cur, "prefix");
	xmlChar* scope = xmlGetProp(cur, "scope");

	for (cur=cur->xmlChildrenNode; cur != NULL; cur=cur->next)
	{
		if (0 == xmlStrcmp(cur->name, "Variable"))
		{
			confValue* pValue = parseVariable(doc, cur, pApp, scope, prefix);

			if (NULL == pValue)
				return FAILURE;

			confApplication_addValue(pApp, pValue);
		}
	}

	SAFE_XMLFREE(prefix);
	SAFE_XMLFREE(scope);

	return SUCCESS;
}
/* }}} */

/* {{{ parseServer
 */
int parseServer(xmlDocPtr doc, xmlNodePtr cur, confApplication* pApp)
{
	int bMe = 1;
	int retval = SUCCESS;
	char* myip = NULL;
	char* ip = xmlGetProp(cur, "ip");
	char* ifname = xmlGetProp(cur, "interface");
	char* hostname = xmlGetProp(cur, "hostname");
	char* domain = xmlGetProp(cur, "domain");
	char myname[MAXHOSTNAMELEN+1];

	if (NULL != ip)
	{
		myip = getifaddress((NULL != ifname) ? ifname : "eth0", myname, sizeof(myname));
		bMe = ((NULL != myip) && (0 == strncmp(ip, myip, strlen(ip))));
	}
	else if (NULL != hostname)
	{
		gethostnamepart(myname, sizeof(myname), "s");
		bMe = (0 == strcmp(myname, hostname));
	}
	else if (NULL != domain)
	{
		gethostnamepart(myname, sizeof(myname), "d");
		bMe = (0 == strcmp(myname, domain));
	}

	if (bMe)
	{
		for (cur=cur->xmlChildrenNode; cur != NULL; cur=cur->next)
		{
			if (0 == xmlStrcmp(cur->name, "Constants"))
				retval = parseConstants(doc, cur, pApp);
			else if (0 == xmlStrcmp(cur->name, "Variables"))
				retval = parseVariables(doc, cur, pApp);

			if (SUCCESS != retval)
				break;
		}
	}

	SAFE_XMLFREE(ip);
	SAFE_XMLFREE(ifname);
	SAFE_XMLFREE(hostname);
	SAFE_XMLFREE(domain);

	return retval;
}
/* }}} */

/* {{{ parseApplication
 */
confApplication* parseApplication(xmlDocPtr doc, xmlNodePtr cur)
{
	xmlChar* name = xmlGetProp(cur, "name");
	xmlChar* ns = xmlGetProp(cur, "namespace");	
	confApplication* pApp = confApplication_new();

	pApp->pszName = (NULL != name) ? strdup(name) : strdup("");
	pApp->pszNamespace = (NULL != ns) ? strdup(ns) : strdup("");
	pApp->bUpperCaseConstants = 1;

    for (cur=cur->xmlChildrenNode; cur != NULL; cur=cur->next)
	{
		int retval = FAILURE;

        if (0 == xmlStrcmp(cur->name, "Server"))
			retval = parseServer(doc, cur, pApp);
        else if (0 == xmlStrcmp(cur->name, "Constants"))
			retval = parseConstants(doc, cur, pApp);
        else if (0 == xmlStrcmp(cur->name, "Variables"))
			retval = parseVariables(doc, cur, pApp);
		else
			php_error(E_ERROR, "Unexpected element %s", cur->name);

		if (SUCCESS != retval)
		{
			confApplication_delete(&pApp);
			pApp = NULL;
			break;
		}
    }

	SAFE_XMLFREE(name);
	SAFE_XMLFREE(ns);

	return pApp;
}
/* }}} */

/* {{{ parsePackage
 */
confApplication* parsePackage(xmlDocPtr doc, xmlNodePtr cur)
{
	// Delegate for now...
	return parseApplication(doc, cur);
}
/* }}} */

//////////////////////////////////////////////////////////////////////////
//
// confEnvironment methods
//
//////////////////////////////////////////////////////////////////////////

/* {{{ confEnvironment_new
 */
confEnvironment* confEnvironment_new()
{
	confEnvironment* pThis = malloc(sizeof(confEnvironment));
	confEnvironment_ctor(pThis);
	return pThis;
}
/* }}} */

/* {{{ confEnvironment_delete
 */
void confEnvironment_delete(confEnvironment** ppThis)
{
	if ((NULL != ppThis) && (NULL != *ppThis))
	{
		confEnvironment_dtor(*ppThis);
		free(*ppThis);
		*ppThis = NULL;
	}
}
/* }}} */

/* {{{ confEnvironment_ctor
 */
void confEnvironment_ctor(confEnvironment* pThis)
{
	memset(pThis, 0, sizeof(confEnvironment));

	pThis->bHasConstants = -1; // don't know
	pThis->bHasVariables = -1; // don't know
}
/* }}} */

/* {{{ confEnvironment_dtor
 */
void confEnvironment_dtor(confEnvironment* pThis)
{
	confApplication* pApp = NULL;
	confApplication* pNextApp = NULL;

	// Free applications
	pApp = pThis->pApps;
	while (NULL != pApp)
	{
		pNextApp = pApp->pNext;
		confApplication_dtor(pApp);
		free(pApp);
		pApp = pNextApp;
	}

	SAFE_FREE(pThis->pszFilename);
	SAFE_FREE(pThis->pszSerial);

	memset(pThis, 0, sizeof(confEnvironment));
}
/* }}} */

/* {{{ confEnvironment_addApplication
 */
void confEnvironment_addApplication(confEnvironment* pThis, confApplication* pApp, int bHead)
{
	confApplication* pEOL = pThis->pApps;

	if (bHead || (NULL == pEOL))
	{
		// Add new element to the head of the list
		pApp->pNext = pThis->pApps;
		pThis->pApps = pApp;
	}
	else
	{
		// Add new element to the end of the list
		while (NULL != pEOL->pNext)
			pEOL = pEOL->pNext;
		pEOL->pNext = pApp;
		pApp->pNext = NULL;
	}

	if (confApplication_hasAnyConstants(pApp))
		pThis->bHasConstants = 1;
	if (confApplication_hasAnyVariables(pApp))
		pThis->bHasVariables = 1;
}
/* }}} */

/* {{{ confEnvironment_hasAnyConstants
 */
int confEnvironment_hasAnyConstants(confEnvironment* pThis)
{
	if ((NULL != pThis) && (NULL != pThis->pApps))
	{
		if (pThis->bHasConstants < 0)
		{
			confApplication* pApp = NULL;
			pThis->bHasConstants = 0;
			for (pApp=pThis->pApps; NULL != pApp; pApp=pApp->pNext)
				if (confApplication_hasAnyConstants(pApp))
					pThis->bHasConstants = 1;
		}

		return pThis->bHasConstants;
	}

	return 0;
}
/* }}} */

/* {{{ confEnvironment_hasAnyVariables
 */
int confEnvironment_hasAnyVariables(confEnvironment* pThis)
{
	if ((NULL != pThis) && (NULL != pThis->pApps))
	{
		if (pThis->bHasVariables < 0)
		{
			confApplication* pApp = NULL;
			pThis->bHasVariables = 0;
			for (pApp=pThis->pApps; NULL != pApp; pApp=pApp->pNext)
				if (confApplication_hasAnyVariables(pApp))
					pThis->bHasVariables = 1;
		}

		return pThis->bHasVariables;
	}

	return 0;
}
/* }}} */

/* {{{ confEnvironment_parseFile(confEnvironment* pThis, const char* pszFilename, const char* pszSerial)
   You can call this method multiple times for the same environment to add
   applications to the environment. The filename and serial tag are updated though. */
int confEnvironment_parseFile(confEnvironment* pThis, const char* pszFilename, const char* pszSerial)
{
	xmlDocPtr doc = NULL;
    xmlNodePtr cur = NULL;

	LIBXML_TEST_VERSION
	xmlKeepBlanksDefault(0);

	if (NULL == pThis->pszFilename)
		pThis->pszFilename = strdup(pszFilename);
	if (NULL == pThis->pszSerial)
		pThis->pszSerial = strdup((pszSerial) ? pszSerial : "");

	doc = xmlParseFile(pThis->pszFilename);
	if (doc == NULL)
	{
		php_error(E_ERROR, "Failed to parse file (%s)", pThis->pszFilename);
		return FAILURE;
	}

    cur = xmlDocGetRootElement(doc);
    if (cur == NULL)
	{
		php_error(E_ERROR, "Failed to find document root (%s)", pThis->pszFilename);
		xmlFreeDoc(doc);
		xmlCleanupParser();
		return FAILURE;
    }

    if (0 != xmlStrcmp(cur->name, "Environments"))
	{
		php_error(E_ERROR, "Document root is not the correct type (%s)", pThis->pszFilename);
		xmlFreeDoc(doc);
		xmlCleanupParser();
		return FAILURE;
    }

    for (cur=cur->xmlChildrenNode; cur != NULL; cur=cur->next)
	{
		confApplication* pApp = NULL;

		if (0 == xmlStrcmp(cur->name, "Application"))
			pApp = parseApplication(doc, cur);
        else if (0 == xmlStrcmp(cur->name, "Package"))
			pApp = parsePackage(doc, cur);
		else
			php_error(E_ERROR, "Unexpected element %s", cur->name);

		if (NULL == pApp)
		{
			confEnvironment_dtor(pThis);
			pThis = NULL;
			break; // abort
		}

		confEnvironment_addApplication(pThis, pApp, 0);
    }

	xmlFreeDoc(doc);
	xmlCleanupParser();

	return (NULL != pThis) ? SUCCESS : FAILURE;
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
