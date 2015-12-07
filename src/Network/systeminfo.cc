/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		April 2010
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";


#include "systeminfo.h"

#include "bufstring.h"
#include "bufstringset.h"
#include "nrbytes2string.h"
#include "checksum.h"
#include "file.h"
#include "filepath.h"
#include "iostrm.h"
#include "oddirs.h"
#ifndef OD_NO_QT
#include "perthreadrepos.h"

#include <QHostAddress>
#include <QHostInfo>
#include <QNetworkInterface>
#endif

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

od_uint64 uniqueSystemID()
{
    BufferStringSet addresses;
    BufferStringSet names;
    macAddresses( names, addresses, true );

    if ( addresses.isEmpty() )
	return 0;

    addresses.sort();

    const BufferString address = addresses.get(0);
    return checksum64( (const unsigned char*) address.buf(), address.size() );
}

const char* localHostName()
{
#ifndef OD_NO_QT
    mDeclStaticString( str );
    str = QHostInfo::localHostName();
    return str.buf();
#else
    return 0;
#endif
}


const char* localAddress()
{ return hostAddress( localHostName() ); }


const char* hostName( const char* ip )
{
#ifndef OD_NO_QT
    mDeclStaticString( str );
    QHostInfo qhi = QHostInfo::fromName( ip );
    str = qhi.hostName();
    if ( str == ip )
	str.setEmpty();
    return str.buf();
#else
    return 0;
#endif
}


const char* hostAddress( const char* hostname )
{
#ifndef OD_NO_QT
    mDeclStaticString( str );
    str.setEmpty();
    QHostInfo qhi = QHostInfo::fromName( hostname );
    QList<QHostAddress> addresses = qhi.addresses();
    for ( int idx=0; idx<addresses.size(); idx++ )
    {
	if ( addresses[idx] == QHostAddress::LocalHost ||
	     addresses[idx].toString().contains(':') ) continue;
	str = addresses[idx].toString();
    }

    return str.buf();
#else
    return 0;
#endif
}


bool lookupHost( const char* host_ip, BufferString* msg )
{
#ifndef OD_NO_QT
    if ( msg )
	msg->set( host_ip ).add( ": " );
    QHostInfo hi = QHostInfo::fromName( host_ip );
    if ( hi.error() != QHostInfo::NoError )
    {
	if ( msg )
	    msg->add( hi.errorString() );
	return false;
    }

    QHostAddress qha;
    const bool isip = qha.setAddress( host_ip );
    if ( isip )
    {
	const BufferString hostname = hostName( host_ip );
	if ( hostname.isEmpty() || hostname==host_ip )
	{
	    if ( msg )
		msg->add( "Not found" );
	    return false;
	}
	if ( msg )
	    msg->add( "Found with hostname " ).add( hostName(host_ip) );
    }
    else if ( msg )
	msg->add( "Found with IP address " ).add( hostAddress(host_ip) );

    return true;
#else
    return false;
#endif
}


void macAddresses( BufferStringSet& names, BufferStringSet& addresses,
		   bool onlyactive )
{
#ifndef OD_NO_QT
    QList<QNetworkInterface> allif = QNetworkInterface::allInterfaces();
    for ( int idx=0; idx<allif.size(); idx++ )
    {
	QNetworkInterface& qni = allif[idx];
	QNetworkInterface::InterfaceFlags flags = qni.flags();
	if ( !qni.isValid() || !flags.testFlag(QNetworkInterface::CanBroadcast)
			  || !flags.testFlag(QNetworkInterface::CanMulticast) )
	    continue;

	if ( onlyactive && ( !flags.testFlag(QNetworkInterface::IsUp)
			     || !flags.testFlag(QNetworkInterface::IsRunning)) )
	    continue;

	if ( qni.name().isEmpty() || qni.hardwareAddress().isEmpty() )
	    continue;

	names.add( qni.name() );
	addresses.add( qni.hardwareAddress() );
    }
#endif
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

    struct statfs fsstatbuf;
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
    if ( !iostrm )
	dir = GetDataDir();
    else
	dir = FilePath( iostrm->fullUserExpr() ).pathOnly();

    return getFreeMBOnDisk( dir );
}


void getFreeMBOnDiskMsg( int mb, uiString& str )
{
    od_uint64 bytes = mb;
    bytes <<= 20;
    NrBytesToStringCreator converter( bytes );
    str = od_static_tr( "getFreeMBOnDiskMsg",
    			"Free space on disk: %1")
	.arg( converter.getString( bytes ) );
}


const char* getFileSystemName( const char* path )
{
    FilePath fp( path );
#ifdef __win__
    const char* drive = fp.winDrive();
    mDefineStaticLocalObject( char, filesystemname, [MAX_PATH+1] = { 0 } );
    char volumename[MAX_PATH+1] = { 0 };

    GetVolumeInformationA( path, volumename, ARRAYSIZE(volumename),
			   0, 0, 0, filesystemname,
			   ARRAYSIZE(filesystemname) );
    return filesystemname;
#else
    pFreeFnErrMsg( "Not implemented yet" );
    return 0;
#endif
}

} // namespace System
