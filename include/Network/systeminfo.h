#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		April 2010
 RCS:		$Id$
________________________________________________________________________

-*/

#include "networkmod.h"
#include "gendefs.h"

class BufferStringSet;
class IOObj;
class uiString;

namespace System
{
    mGlobal(Network) const char*	localHostName();
					//!< shortcut to GetLocalHostName()
    mGlobal(Network) const char*	localAddress(bool ipv4only=true);

    mGlobal(Network) const char*	hostName(const char* ip);
    mGlobal(Network) const char*	hostAddress(const char* hostname,
						    bool ipv4only=true);
    mGlobal(Network) bool		lookupHost(const char* host_ip,
						   BufferString* msg=0);
    mGlobal(Network) bool		isValidIPAddress(const char* host_ip);

    mGlobal(Network) void		macAddresses(BufferStringSet& names,
						    BufferStringSet& addresses,
						    bool onlyactive=false);

    mGlobal(Network) int		getFreeMBOnDisk(const char* path);
    mGlobal(Network) int		getFreeMBOnDisk(const IOObj&);
    mGlobal(Network) void		getFreeMBOnDiskMsg(int,BufferString&);
    mGlobal(Network) void		getFreeMBOnDiskMsg(int,uiString&);
    mGlobal(Network) const char*	getFileSystemName(const char* path);

    mGlobal(Network) od_int64		bytesAvailable(const char* path);
    mGlobal(Network) od_int64		bytesFree(const char* path);
    mGlobal(Network) od_int64		bytesTotal(const char* path);
    mGlobal(Network) const char*	fileSystemName(const char* path);
    mGlobal(Network) const char*	fileSystemType(const char* path);

    mGlobal(Network) od_uint64		macAddressHash();
					/*!<Returns the checksum of the first
					    'valid' mac address. */

    mGlobal(Network) od_uint64		uniqueSystemID();
					/*!<Deprecated. Please use
					    macAddressHash(). */
    mGlobal(Network) bool		getHostIDs(BufferStringSet& hostids,
						   BufferString& errmsg);
    mGlobal(Network) const char*	productName();
					//!<return OS, distribution, etc.
    mGlobal(Network) const char*	kernelVersion();
}

