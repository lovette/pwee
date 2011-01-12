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

#ifndef PWEE_IF_H
#define PWEE_IF_H

#define IFINFONAME		16 // Must be >= IFNAMSIZ
#define IFINFOADDR		16
#define IFINFOHWADDR	6

// Describes a single interface
typedef struct _ifinfo
{
	char szifname[IFINFONAME+1]; // Interface name (lo, eth0, eth1);
	char szifaddr[IFINFOADDR+1]; // Interface address xxx.xxx.xxx.xxx
} ifinfo;

// Describes a list of interfaces
typedef struct _ifcache
{
	int ifcount;    // Number of interfaces in iflist
	ifinfo* iflist; // Array of interfaces
} ifcache;

// ifcache methods
ifcache* ifcache_new();
void ifcache_delete(ifcache** ppThis);
void ifcache_ctor(ifcache* pThis);
void ifcache_dtor(ifcache* pThis);
int ifcache_enuminterfaces(ifcache* pThis);

// Public functions
char* getifaddress(const char* ifname, char* ifaddr, int ifaddrlen);
char* gethostnamepart(char* name, int len, const char* part);
char* getifhwaddress(const char* ifname, char* ifhwaddr);

#endif	/* PWEE_IF_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 */
