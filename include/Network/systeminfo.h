#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
    mGlobal(Network) const char*	localFullHostName();
					/*!< Unlike localHostName,
					     always returns a fully qualified
					     domain name */
    mGlobal(Network) const char*	localDomainName();
					/*!< The domain name of the local host
					      May be empty */
    mGlobal(Network) const char*	localHostNameWoDomain();
					/*!< Short version of the host name,
					     always without domain name.
					     Should not be used to make a
					     connection */
    mGlobal(Network) const char*	localAddress(bool ipv4only=true);
    mGlobal(Network) bool		isLocalAddressInUse(
						const char* ipaddr=nullptr);

    mGlobal(Network) const char*	hostName(const char* ip);
    mGlobal(Network) const char*	hostAddress(const char* hostname,
						    bool ipv4only=true);
    mGlobal(Network) bool		lookupHost(const char* host_ip,
						   BufferString* msg=nullptr);
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

    mGlobal(Network) BufferString	macAddressHash();
					/*!<Returns the sha3-256 sum of the
					    first 'valid' mac address. */

    mDeprecated("Use System::macAddressHash")
    mGlobal(Network) BufferString	uniqueSystemID();
					/*!<Deprecated. Please use
					    macAddressHash(). */
    mGlobal(Network) bool		getHostIDs(BufferStringSet& hostids,
						   BufferString& errmsg);
    mGlobal(Network) const char*	productName();
					//!<return OS, distribution, etc.
    mGlobal(Network) const char*	kernelVersion();
    mGlobal(Network) const IOPar&	graphicsInformation();

} // namespace System
