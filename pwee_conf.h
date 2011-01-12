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

#ifndef PWEE_CONF_H
#define PWEE_CONF_H

#define PWEE_VALUESCOPE_REQUEST		0
#define PWEE_VALUESCOPE_EXECUTOR	1

typedef struct _confValue {
	pweeString strValueName;		// Variable name
	zend_uchar type;				// Variable type (IS_LONG, IS_DOUBLE, IS_BOOL, IS_STRING)
	zvalue_value value;				// Variable value
	zend_bool bConstant;			// Is this a variable or constant value?
	int scope;						// PWEE_VALUESCOPE_*
	struct _confValue* pNext;		// Next variable
} confValue;

typedef struct _confApplication {
	pweeString strAppName;			// Application name
	pweeString strNamespace;		// Application variable and constant name prefix
	confValue* pVariables;			// First variable - one list for each PWEE_VALUESCOPE
	confValue* pConstants;			// First constant
	zend_bool bUpperCaseConstants;	// Do we uppercase constants?
	struct _confApplication* pNext;	// Next appliation
} confApplication;

typedef struct _confEnvironment {
	confApplication* pApps;			// First appliation
	char* pszFilename;				// Path to XML
	char* pszSerial;				// Application defined identifier
	int bHasConstants;				// Allows quick check for constants
	int bHasVariables;				// Allows quick check for variables
} confEnvironment;

// Global helpers
int stringToScope(const char* scope);
const char* scopeToString(int scope);

// confValue methods
confValue* confValue_new();
void confValue_delete(confValue** ppThis);
void confValue_ctor(confValue* pThis);
void confValue_dtor(confValue* pThis);
int confValue_setValue(confValue* pThis, pweeString* pstrName, pweeString* pstrValue, const char* type, zend_bool bConstant, confApplication* pApp);
int confValue_setValueFromZval(confValue* pThis, zval* pz);
char* confValue_getValueAsString(confValue* pThis);
char* confValue_getTypeAsString(confValue* pThis);

// confApplication methods
confApplication* confApplication_new();
void confApplication_delete(confApplication** ppThis);
void confApplication_ctor(confApplication* pThis);
void confApplication_dtor(confApplication* pThis);
void confApplication_addValue(confApplication* pThis, confValue* pValue);
int confApplication_hasAnyConstants(confApplication* pThis);
int confApplication_hasAnyVariables(confApplication* pThis);

// confEnvironment methods
confEnvironment* confEnvironment_new();
void confEnvironment_delete(confEnvironment** ppThis);
void confEnvironment_ctor(confEnvironment* pThis);
void confEnvironment_dtor(confEnvironment* pThis);
int confEnvironment_parseFile(confEnvironment* pThis, const char* pszFilename, const char* pszSerial);
void confEnvironment_addApplication(confEnvironment* pThis, confApplication* pApp, int bHead);
int confEnvironment_hasAnyConstants(confEnvironment* pThis);
int confEnvironment_hasAnyVariables(confEnvironment* pThis);

#endif	/* PWEE_CONF_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 */
