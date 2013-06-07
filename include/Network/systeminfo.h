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

#include "gendefs.h"

class BufferString;
class BufferStringSet;
class IOObj;

namespace System
{
    mGlobal const char*		localHostName();
    mGlobal const char*		localAddress();

    mGlobal const char*		hostName(const char* ip);
    mGlobal const char*		hostAddress(const char* hostname);

    mGlobal void		macAddresses(BufferStringSet& names,
					     BufferStringSet& addresses);

    mGlobal int			getFreeMBOnDisk(const char* path);
    mGlobal int			getFreeMBOnDisk(const IOObj&);
    mGlobal void		getFreeMBOnDiskMsg(int,BufferString&);
    mGlobal const char*		getFileSystemName(const char* path);
}

#endif
