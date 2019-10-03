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
#include "genc.h"
#include "iostrm.h"
#include "oddirs.h"
#ifndef OD_NO_QT
# include "perthreadrepos.h"

# include <QHostAddress>
# include <QHostInfo>
# include <QNetworkInterface>
# include <QStorageInfo>
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

static bool isAcceptable( const QHostAddress& addr, bool ipv4only )
{
    if ( addr.isNull() )
	return false;

    const QAbstractSocket::NetworkLayerProtocol protocol = addr.protocol();
#if QT_VERSION >= 0x050B00
    if ( addr.isGlobal() )
	return ipv4only ? protocol == QAbstractSocket::IPv4Protocol : true;
#elif QT_VERSION >= 0x050600
    if ( addr.isMulticast() )
	return false;
#elif QT_VERSION >= 0x050000
    if ( addr.isLoopback() )
	return false;
#endif

    return ipv4only ? protocol == QAbstractSocket::IPv4Protocol
		    : protocol > QAbstractSocket::UnknownNetworkLayerProtocol;
}


od_uint64 macAddressHash()
{
    BufferStringSet addresses;
    BufferStringSet names;
    macAddresses( names, addresses, true );

    const char* virtboxaddress = "0A:00:27:00:00:00";
    const int virtboxidx = addresses.indexOf( virtboxaddress );
    if ( addresses.validIdx(virtboxidx) )
	addresses.removeSingle( virtboxidx );

    if ( addresses.isEmpty() )
	return 0;

    addresses.sort(); // Not really needed, but leave it in for compatibility
    const BufferString& address = addresses.get( 0 );
    return checksum64( (const unsigned char*)address.buf(), address.size() );
}


od_uint64 uniqueSystemID()
{ return macAddressHash(); }


const char* localHostName()
{
    mDeclStaticString( str );
    str = GetLocalHostName();
    return str.buf();
}


const char* localAddress( bool ipv4only )
{
    mDeclStaticString( str );
    str = hostAddress( localHostName() );
    if ( !str.isEmpty() )
	return str.buf();

    // Fallback implementation for some new OS/hardware
    const QList<QHostAddress> addresses = QNetworkInterface::allAddresses();
    for ( int idx=0; idx<addresses.size(); idx++ )
    {
	const QHostAddress& addr = addresses[idx];
	if ( !isAcceptable(addr,ipv4only) )
	    continue;

	str = addr.toString();
	break;
    }

    return str.buf();
}


const char* hostName( const char* ip )
{
#ifndef OD_NO_QT
    mDeclStaticString( str );
    const QHostInfo qhi = QHostInfo::fromName( ip );
    str = qhi.hostName();
    if ( str == ip )
	str.setEmpty();
    return str.buf();
#else
    return 0;
#endif
}


const char* hostAddress( const char* hostname, bool ipv4only )
{
#ifndef OD_NO_QT
    mDeclStaticString( str );
    str.setEmpty();
    const QHostInfo qhi = QHostInfo::fromName( QString(hostname) );
    const QList<QHostAddress> addresses = qhi.addresses();
    for ( int idx=0; idx<addresses.size(); idx++ )
    {
	const QHostAddress& addr = addresses[idx];
	if ( !isAcceptable(addr,ipv4only) )
	    continue;

	str = addr.toString();
	break;
    }

    return str.buf();
#else
    return nullptr;
#endif
}


bool lookupHost( const char* host_ip, BufferString* msg )
{
#ifndef OD_NO_QT
    if ( msg )
	msg->set( host_ip ).add( ": " );
    const QHostInfo qhi = QHostInfo::fromName( QString(host_ip) );
    if ( qhi.error() != QHostInfo::NoError )
    {
	if ( msg )
	    msg->add( qhi.errorString() );
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


od_int64 bytesAvailable( const char* path )
{
#ifndef OD_NO_QT
    const QStorageInfo storageinfo( path );
    return storageinfo.bytesAvailable();
#else
    return -1;
#endif
}


od_int64 bytesFree( const char* path )
{
#ifndef OD_NO_QT
    const QStorageInfo storageinfo( path );
    return storageinfo.bytesFree();
#else
    return -1;
#endif
}


od_int64 bytesTotal( const char* path )
{
#ifndef OD_NO_QT
    const QStorageInfo storageinfo( path );
    return storageinfo.bytesTotal();
#else
    return -1;
#endif
}


const char* fileSystemName( const char* path )
{
#ifndef OD_NO_QT
    mDeclStaticString( str );
    const QStorageInfo storageinfo( path );
    str = storageinfo.name();
    if ( str.isEmpty() )
	str = storageinfo.displayName();
    return str.buf();
#else
    return 0;
#endif
}


const char* fileSystemType( const char* path )
{
#ifndef OD_NO_QT
    mDeclStaticString( str );
    const QStorageInfo storageinfo( path );
    str = storageinfo.fileSystemType().constData();
    return str.buf();
#else
    return 0;
#endif
}


#define mToKbFac (1.0 / 1024.0)

int getFreeMBOnDisk( const char* path )
{
#ifndef OD_NO_QT
    const QStorageInfo storageinfo( path );
    const od_int64 bytesavail = storageinfo.bytesAvailable();
    return (int)(bytesavail/1024/1024);
#else
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
#endif
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


void getFreeMBOnDiskMsg( int mb, BufferString& bs )
{
    bs = "Free space on disk: ";
    od_uint64 bytes = mb;
    bytes <<= 20;
    NrBytesToStringCreator converter( bytes );
    bs += converter.getString( bytes );
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
