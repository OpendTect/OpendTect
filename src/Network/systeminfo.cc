/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		April 2010
________________________________________________________________________

-*/



#include "systeminfo.h"

#include "bufstring.h"
#include "bufstringset.h"
#include "nrbytes2string.h"
#include "checksum.h"
#include "file.h"
#include "filepath.h"
#include "genc.h"
#include "generalinfo.h"
#include "iostrm.h"
#include "oddirs.h"
#include "staticstring.h"

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


const char* localHostName()
{
    mDeclStaticString( str );
    str = GetLocalHostName();
    return str.buf();
}


const char* localAddress( bool ipv4only )
{
    mDeclStaticString( str );
    str = hostAddress( localHostName(), ipv4only );
    if ( !str.isEmpty() )
	return str.buf();

    // Fallback implementation for some new OS/hardware
    const QList<QHostAddress> addresses = QNetworkInterface::allAddresses();
    for ( const auto addr : addresses )
    {
	if ( !isAcceptable(addr,ipv4only) )
	    continue;

	str = addr.toString();
	break;
    }

    return str.buf();
}


const char* hostName( const char* ip )
{
    mDeclStaticString( str );
    const QHostInfo qhi = QHostInfo::fromName( ip );
    str = qhi.hostName();
    if ( str == ip )
	str.setEmpty();
    return str.buf();
}


const char* hostAddress( const char* hostname, bool ipv4only )
{
    mDeclStaticString( str );
    str.setEmpty();
    const QHostInfo qhi = QHostInfo::fromName( QString(hostname) );
    const QList<QHostAddress> addresses = qhi.addresses();
    for ( const auto addr : addresses )
    {
	if ( !isAcceptable(addr,ipv4only) )
	    continue;

	str = addr.toString();
	break;
    }

    return str.buf();
}


bool lookupHost( const char* host_ip, BufferString* msg )
{
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
}


void macAddresses( BufferStringSet& names, BufferStringSet& addresses,
		   bool onlyactive )
{
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
	dir = File::Path( iostrm->mainFileName() ).pathOnly();

    return getFreeMBOnDisk( dir );
}


void getFreeMBOnDiskMsg( int mb, uiString& str )
{
    od_int64 bytes = mb;
    bytes <<= 20;
    NrBytesToStringCreator converter( bytes );
    str = od_static_tr( "getFreeMBOnDiskMsg",
			"Free space on disk: %1")
	.arg( converter.getString( bytes ) );
}


void getFreeMBOnDiskUiMsg( int mb, uiString& msg )
{
    msg = od_static_tr("getFreeMBOnDiskUiMsg","Free space on disk:");
    if( mb < 1024 )
	msg = od_static_tr("getFreeMBOnDiskUiMsg","%1 %2 MB").arg(msg).arg(mb);
    else
    {
	int gb = mb / 1024;
	float fmb = (mb % 1024) / 102.4;
	int tenthsofgb = mNINT32(fmb);
	msg = od_static_tr("getFreeMBOnDiskUiMsg","%1 %2.%3 GB").arg(msg)
						    .arg(gb).arg(tenthsofgb);
    }
}


const char* getFileSystemName( const char* path )
{
    File::Path fp( path );
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

bool getHostIDs( BufferStringSet& hostids, BufferString& errmsg )
{
    hostids.setEmpty();
    return OD::getHostIDs( hostids, errmsg );
}

} // namespace System
