#ifndef systeminfo_h
#define systeminfo_h

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

class BufferString;
class BufferStringSet;
class IOObj;

namespace System
{
    mGlobal(Network) const char*		localHostName();
    mGlobal(Network) const char*		localAddress();

    mGlobal(Network) const char*		hostName(const char* ip);
    mGlobal(Network) const char*		hostAddress(const char* hostname);

    mGlobal(Network) void		macAddresses(BufferStringSet& names,
					     BufferStringSet& addresses);

    mGlobal(Network) int			getFreeMBOnDisk(const char* path);
    mGlobal(Network) int			getFreeMBOnDisk(const IOObj&);
    mGlobal(Network) void		getFreeMBOnDiskMsg(int,BufferString&);
    mGlobal(Network) const char*		getFileSystemName(const char* path);
}

#endif

