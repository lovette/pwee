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

#include <netdb.h>
#include <net/if.h>
#include <arpa/inet.h>

#ifdef HAVE_IOCTL
#include <sys/ioctl.h>
#endif

struct ifconf* enuminterfaces(struct ifconf* pifc);

//////////////////////////////////////////////////////////////////////////
//
// ifcache methods
//
//////////////////////////////////////////////////////////////////////////

/* {{{ ifcache* ifcache_new()
   Allocates and initializes a new ifcache structure. */
ifcache* ifcache_new()
{
	ifcache* pThis = malloc(sizeof(ifcache));
	ifcache_ctor(pThis);
	return pThis;
}
/* }}} */

/* {{{ void ifcache_delete(ifcache** ppThis)
 */
void ifcache_delete(ifcache** ppThis)
{
	if ((NULL != ppThis) && (NULL != *ppThis))
	{
		ifcache_dtor(*ppThis);
		free(*ppThis);
		*ppThis = NULL;
	}
}
/* }}} */

/* {{{ void ifcache_ctor(ifcache* pThis)
   Initialize an ifcache structure. */
void ifcache_ctor(ifcache* pThis)
{
	memset(pThis, 0, sizeof(ifcache));
}
/* }}} */

/* {{{ void ifcache_dtor(ifcache* pThis)
   Clean up an ifcache structure. */
void ifcache_dtor(ifcache* pThis)
{
	if (NULL != pThis->iflist)
		free(pThis->iflist);

	memset(pThis, 0, sizeof(ifcache));
}
/* }}} */

/* {{{ int ifcache_enuminterfaces(ifcache* pThis)
   Initializes an ifcache with each active network interface. */
int ifcache_enuminterfaces(ifcache* pThis)
{
	int i;
	char* pszntoa = NULL;
	struct ifreq *ifr = NULL;
	struct ifconf ifc;

	memset(pThis, 0, sizeof(ifcache));

	if (NULL == enuminterfaces(&ifc))
	{
		return FAILURE;
	}

	pThis->ifcount = ifc.ifc_len / sizeof(struct ifreq);
	pThis->iflist = malloc(pThis->ifcount * sizeof(ifinfo));

	memset(pThis->iflist, 0, pThis->ifcount * sizeof(ifinfo));

	for (i=0, ifr=ifc.ifc_req; i < pThis->ifcount; ifr++, i++)
	{
		pszntoa = inet_ntoa(((struct sockaddr_in *) &ifr->ifr_addr)->sin_addr);
		PHP_STRLCPY(pThis->iflist[i].szifname, ifr->ifr_name, IFINFONAME, strlen(ifr->ifr_name));
		PHP_STRLCPY(pThis->iflist[i].szifaddr, pszntoa, IFINFOADDR, strlen(pszntoa));
	}

	free(ifc.ifc_buf);

	return SUCCESS;
}
/* }}} */

//////////////////////////////////////////////////////////////////////////
//
// interface functions
//
//////////////////////////////////////////////////////////////////////////

/* {{{ char* getifhwaddress(char* ifname, char* ifhwaddr)
   Returns the hardware address (MAC) of an active network interface.
   ifname is the name of the interface. This is usually a driver name followed
   by a unit number, for example eth0 for the first Ethernet interface.
   ifaddr should be a buffer that can hold at least IFINFOHWADDR characters
   plus the terminating `\0' character.
   Returns NULL if the address cannot be determined. */
char* getifhwaddress(const char* ifname, char* ifhwaddr)
{
#ifdef HAVE_IOCTL
	int fd = 0;
	struct ifreq ifr;

	memset (&ifr, 0, sizeof(ifr));
	memset (ifhwaddr, 0, IFINFOHWADDR+1);

	PHP_STRLCPY(ifr.ifr_name, ifname, IFNAMSIZ, strlen(ifname));
	
	fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
	if (fd > 0)
	{
		if (0 == ioctl(fd, SIOCGIFHWADDR, &ifr))
		{
			unsigned char* a = (unsigned char *) &ifr.ifr_hwaddr.sa_data;
			if (a[0] + a[1] + a[2] + a[3] + a[4] + a[5])
				memcpy(ifhwaddr, a, IFINFOHWADDR);
		}
	
		close(fd);
	}
#else
	PHP_STRLCPY(ifhwaddr, "NA", IFINFOHWADDR+1, 2);
#endif // HAVE_IOCTL

	return (*ifhwaddr) ? ifhwaddr : NULL;
}
/* }}} */

/* {{{ char* getifaddress(char* ifname, char* addr, int addrlen)
   Returns the IP address of an active network interface. ifname is the name
   of the interface. This is usually a driver name followed by a unit number, for
   example eth0 for the first Ethernet interface. ifaddr should be a buffer
   that can hold at least IFNAMSIZ characters. Returns NULL if the address
   cannot be determined. */
char* getifaddress(const char* ifname, char* ifaddr, int ifaddrlen)
{
#ifdef HAVE_IOCTL
	int fd = 0;
	struct ifreq ifr;

	memset (&ifr, 0, sizeof(ifr));
	memset (ifaddr, 0, ifaddrlen);

	PHP_STRLCPY(ifr.ifr_name, ifname, IFNAMSIZ, strlen(ifname));
	
	fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
	if (fd > 0)
	{
		if (0 == ioctl(fd, SIOCGIFADDR, &ifr))
		{
			char* ntoa = inet_ntoa(((struct sockaddr_in *) &ifr.ifr_addr)->sin_addr);
			if (NULL != ntoa)
				PHP_STRLCPY(ifaddr, ntoa, ifaddrlen, strlen(ntoa));
		}
	
		close(fd);
	}
#else
	PHP_STRLCPY(ifaddr, "NA", ifaddrlen, 2);
#endif // HAVE_IOCTL
	
	return (*ifaddr) ? ifaddr : NULL;
}
/* }}} */

/* {{{ struct ifconf* enuminterfaces(struct ifconf* pifc)
   Returns a list of raw interface configurations.
   Returns pifc if successful or NULL on failure.
   If successful ifc_buf will point to an array of ifreq structures.
   The caller is responsible for freeing the ifc_buf buffer. */
struct ifconf* enuminterfaces(struct ifconf* pifc)
{
#ifdef HAVE_IOCTL
	int fd = 0;
	int ifc_req_num = 2;
	int ifc_len_guess = 0;

	memset(pifc, 0, sizeof(*pifc));

	fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
	if (fd > 0)
	{
		do
		{
			ifc_len_guess = (++ifc_req_num * sizeof(struct ifreq));

			pifc->ifc_len = ifc_len_guess;
			pifc->ifc_buf = realloc(pifc->ifc_buf, pifc->ifc_len);

			if ((NULL == pifc->ifc_buf) || (0 != ioctl(fd, SIOCGIFCONF, pifc)))
			{
				if (pifc->ifc_buf)
					free(pifc->ifc_buf);
				pifc->ifc_buf = NULL;
			}

		} while (pifc->ifc_buf && (ifc_len_guess == pifc->ifc_len));

		close(fd);
	}

	return (pifc->ifc_buf) ? pifc : NULL;
#else
	return NULL;
#endif // HAVE_IOCTL
}
/* }}} */

//////////////////////////////////////////////////////////////////////////
//
// hostname functions
//
//////////////////////////////////////////////////////////////////////////

/* {{{ char* gethostnamepart(char* name, int len, char* part)
   Returns the host, domain or full domain name of the system.
   The part parameter determines what is returned:
   - "s" returns the short name (see hostname -s)
   - "d" returns the domain name (see hostname -d)
   - "f" returns the full name (see hostname -f)
   Returns name if successful or NULL on failure.
   The buffer will be empty on failure.
  */
char* gethostnamepart(char* name, int len, const char* part)
{
	char *p = NULL;
	char myname[MAXHOSTNAMELEN+1];
	struct hostent* he = NULL;

	memset(name, 0, len);

	if (0 != gethostname(myname, sizeof(myname)))
		return NULL;

	he = gethostbyname(myname);
	if (NULL == he)
		return NULL;
	
	if (*part == 'f') 
	{
		PHP_STRLCPY(name, he->h_name, len, strlen(he->h_name));
		return name;
	}
	
	if (*part == 'd') 
	{
		p = strchr(he->h_name, '.'); // Find start of domain name
		if (NULL != p)
		{
			PHP_STRLCPY(name, ++p, len, strlen(p));
			return name;
		}
	}
	
	if (*part == 's') 
	{
		p = strchr(he->h_name, '.'); // Find start of domain name
		if (NULL != p)
			*p = '\0';
		PHP_STRLCPY(name, he->h_name, len, strlen(he->h_name));
		return name;
	}

	return NULL;
}
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 */
