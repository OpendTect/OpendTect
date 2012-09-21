/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		April 2010
________________________________________________________________________

-*/

static const char* rcsID mUnusedVar = "$Id$";


#include "systeminfo.h"

#include "bufstring.h"
#include "bufstringset.h"
#include "errh.h"
#include "file.h"
#include "filepath.h"
#include "iostrm.h"
#include "oddirs.h"

#include <QHostAddress>
#include <QHostInfo>
#include <QNetworkInterface>

#ifdef __lux__
# include <sys/statfs.h>
#endif

#ifdef __mac__
# include <sys/mount.h>
#endif

#ifdef __win__
# include <windows.h>
#endif

namespace System
{

const char* localHostName()
{
    static BufferString str;
    str = QHostInfo::localHostName().toAscii().constData();
    return str.buf();
}


const char* localAddress()
{ return hostAddress( localHostName() ); }


const char* hostName( const char* ip )
{
    static BufferString str;
    QHostInfo qhi = QHostInfo::fromName( ip );
    str = qhi.hostName().toAscii().constData();
    return str.buf();
}


const char* hostAddress( const char* hostname )
{
    static BufferString str;
    QHostInfo qhi = QHostInfo::fromName( hostname );
    QList<QHostAddress> addresses = qhi.addresses();
    for ( int idx=0; idx<addresses.size(); idx++ )
    {
	if ( addresses[idx] == QHostAddress::LocalHost ||
	     addresses[idx].toString().contains(':') ) continue;
	str = addresses[idx].toString().toAscii().constData();
    }

    return str.buf();
}


void macAddresses( BufferStringSet& names, BufferStringSet& addresses )
{
    QList<QNetworkInterface> allif = QNetworkInterface::allInterfaces();
    for ( int idx=0; idx<allif.size(); idx++ )
    {
	QNetworkInterface& qni = allif[idx];
	QNetworkInterface::InterfaceFlags flags = qni.flags();
	if ( !flags.testFlag(QNetworkInterface::CanBroadcast) )
	    continue;

	names.add( qni.name().toAscii().constData() );
	addresses.add( qni.hardwareAddress().toAscii().constData() );
    }
}


#define mToKbFac (1.0 / 1024.0)

int getFreeMBOnDisk( const char* path )
{
    if ( !File::exists(path) )
	return 0;

    const double fac = mToKbFac;
    double res;

#ifdef __win__
    ULARGE_INTEGER freeBytesAvail2User;
    ULARGE_INTEGER totalNrBytes;
    ULARGE_INTEGER totalNrFreeBytes;
    GetDiskFreeSpaceExA( path, &freeBytesAvail2User,
			&totalNrBytes, &totalNrFreeBytes );

    res = freeBytesAvail2User.QuadPart * fac * fac;
#else

    static struct statfs fsstatbuf;
    if ( statfs(path,&fsstatbuf) == -1 )
	return 0;

    res = fac * fac             /* to MB */
	* fsstatbuf.f_bavail    /* available blocks */
	* fsstatbuf.f_bsize;    /* block size */

#endif

    return (int)(res + .5);
}


int getFreeMBOnDisk( const IOObj& ioobj )
{
    mDynamicCastGet(const IOStream*,iostrm,&ioobj)

    BufferString dir;
    if ( !iostrm || iostrm->isCommand() )
	dir = GetDataDir();
    else
	dir = FilePath( iostrm->getExpandedName(true) ).pathOnly();

    return getFreeMBOnDisk( dir );
}


void getFreeMBOnDiskMsg( int mb, BufferString& bs )
{
    bs = "Free space on disk: ";
    if ( mb < 1024 )
	{ bs += mb; bs += " MB"; }
    else
    {
	int gb = mb / 1024;
	bs += gb; bs += ".";
	float fmb = (mb % 1024) / 102.4;
	int tenthsofgb = mNINT32(fmb);
	bs += tenthsofgb; bs += " GB";
    }
}



const char* getFileSystemName( const char* path )
{
    FilePath fp( path );
#ifdef __win__
    const char* drive = fp.winDrive();
    static char filesystemname[MAX_PATH+1] = { 0 };
    char volumename[MAX_PATH+1] = { 0 };

    GetVolumeInformationA( path, volumename, ARRAYSIZE(volumename),
			   0, 0, 0, filesystemname,
			   ARRAYSIZE(filesystemname) );  
    return filesystemname;
#else
    pFreeFnErrMsg( "Not implemented yet", "System::getFileSystemName" );
    return 0;
#endif
}

} // namespace System
