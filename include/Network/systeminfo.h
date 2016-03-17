#ifndef systeminfo_h
#define systeminfo_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		April 2010
________________________________________________________________________

-*/

#include "networkmod.h"
#include "gendefs.h"
#include "uistrings.h"

class BufferStringSet;
class IOObj;
class uiString;

namespace System
{
    mGlobal(Network) const char*	localHostName();
    mGlobal(Network) const char*	localAddress();

    mGlobal(Network) const char*	hostName(const char* ip);
    mGlobal(Network) const char*	hostAddress(const char* hostname);
    mGlobal(Network) bool		lookupHost(const char* host_ip,
						   BufferString* msg=0);

    mGlobal(Network) void		macAddresses(BufferStringSet& names,
						    BufferStringSet& addresses,
						    bool onlyactive=false);

    mGlobal(Network) int		getFreeMBOnDisk(const char* path);
    mGlobal(Network) int		getFreeMBOnDisk(const IOObj&);
    mGlobal(Network) void		getFreeMBOnDiskMsg(int,uiString&);
    mGlobal(Network) void		getFreeMBOnDiskUiMsg(int,uiString&);
    mGlobal(Network) const char*	getFileSystemName(const char* path);

    mGlobal(Network) od_uint64		uniqueSystemID();
					/*!<Returns a string that is unique to
					    this computer. It is based on a
					    scrambled version of the mac-address
					*/
}

#endif
