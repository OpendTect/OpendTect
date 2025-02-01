/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "systeminfo.h"

#include "bufstring.h"
#include "bufstringset.h"
#include "nrbytes2string.h"
#include "file.h"
#include "filepath.h"
#include "genc.h"
#include "generalinfo.h"
#include "iostrm.h"
#include "odcommonenums.h"
#include "oddirs.h"
#include "odplatform.h"
#include "perthreadrepos.h"
#include "settingsaccess.h"
#include "winutils.h"

#include <QHostAddress>
#include <QHostInfo>
#include <QNetworkInterface>
#include <QStorageInfo>
#include <QSysInfo>

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

    if ( addr.isInSubnet(QHostAddress("172.16.0.0"),12) )
	return false;

    const QAbstractSocket::NetworkLayerProtocol protocol = addr.protocol();
#if QT_VERSION >= QT_VERSION_CHECK(5,11,0)
    if ( addr.isGlobal() )
	return ipv4only ? protocol == QAbstractSocket::IPv4Protocol
			: protocol > QAbstractSocket::IPv4Protocol;
#elif QT_VERSION >= QT_VERSION_CHECK(5,6,0)
    if ( addr.isMulticast() )
	return false;
#elif QT_VERSION >= QT_VERSION_CHECK(5,0,0)
    if ( addr.isLoopback() )
	return false;
#endif

    return ipv4only ? protocol == QAbstractSocket::IPv4Protocol
		    : protocol > QAbstractSocket::IPv4Protocol;
}


BufferString macAddressHash()
{
    BufferStringSet addresses, names;
    macAddresses( names, addresses, true );
    if ( addresses.isEmpty() )
	return BufferString::empty();

    static const char* virtboxaddress = "0A:00:27:00:00:00";
    const int virtboxidx = addresses.indexOf( virtboxaddress );
    if ( addresses.validIdx(virtboxidx) )
	addresses.removeSingle( virtboxidx );

    if ( addresses.isEmpty() )
	return BufferString::empty();

    addresses.sort(); // Not really needed, but leave it in for compatibility
    const BufferString& address = *addresses.first();
    const BufferString ret = address.getHash( Crypto::Algorithm::Sha3_256 );
    return ret;
}


BufferString uniqueSystemID()
{
    return macAddressHash();
}


const char* localHostName()
{
    mDeclStaticString( str );
    if ( str.isEmpty() )
	str = GetLocalHostName();
    return str.buf();
}

extern "C" { mGlobal(Basic) void SetLocalHostNameOverrule(const char*); }

const char* localFullHostName()
{
    mDeclStaticString( str );
    if ( str.isEmpty() )
    {
	str = localHostName();
	const BufferString hostnmoverrule =
				SettingsAccess().getHostNameOverrule();
	if ( !hostnmoverrule.isEmpty() )
	{
	    const BufferString ipaddr( hostAddress( hostnmoverrule ) );
	    if ( isValidIPAddress(ipaddr) && ipaddr == localAddress() )
	    {
		str = hostnmoverrule;
		SetLocalHostNameOverrule( str.buf() );
		return str.buf();
	    }
	}

	const char* domainnm = localDomainName();
	if ( domainnm && *domainnm && !str.endsWith(domainnm) )
	    str.add( "." ).add( domainnm );
    }
    if ( __iswin__ )
	str.toLower();

    return str.buf();
}


const char* localDomainName()
{
    mDeclStaticString( str );
    if ( str.isEmpty() )
	str.set( QHostInfo::localDomainName() );
    return str.buf();
}


const char* localHostNameWoDomain()
{
    mDeclStaticString( str );
    if ( str.isEmpty() )
    {
	str.set( localHostName() );
	const char* domainnm = localDomainName();
	if ( domainnm && *domainnm && str.endsWith(domainnm) )
	    str.replace( '.', '\0' );
    }
    return str.buf();
}


const char* localAddress( bool ipv4only )
{
    mDeclStaticString( str );
    str.setEmpty(); // Network configuration may change during runtime

    const QList<QNetworkInterface> allif = QNetworkInterface::allInterfaces();
    QHostAddress ethaddr, wifiaddr, loopbackaddr, otheraddr;
    for ( const auto& qni : allif )
    {
	if ( !qni.isValid() )
	    continue;

	const QNetworkInterface::InterfaceFlags flags = qni.flags();
	if ( !flags.testFlag(QNetworkInterface::IsUp) ||
	     !flags.testFlag(QNetworkInterface::IsRunning) )
	    continue;

	const QList<QNetworkAddressEntry> entries = qni.addressEntries();
	if ( entries.isEmpty() )
	    continue;

	const QNetworkInterface::InterfaceType typ = qni.type();
	for ( const auto& ent : entries )
	{
	    const QHostAddress addr = ent.ip();
	    if ( !isAcceptable(addr,ipv4only) )
		continue;

	    if ( typ == QNetworkInterface::Ethernet )
	    {
		if ( ethaddr.isNull() )
		    ethaddr = addr;
		break;
	    }

	    if ( typ == QNetworkInterface::Wifi ||
		 typ == QNetworkInterface::Ieee80211 )
	    {
		if ( wifiaddr.isNull() )
		    wifiaddr = addr;
		break;
	    }

	    if ( typ == QNetworkInterface::Loopback )
	    {
		if ( loopbackaddr.isNull() )
		    loopbackaddr = addr;
		break;
	    }

	    if ( otheraddr.isNull() )
		otheraddr = addr;

	    break;
	}
    }

    if ( !ethaddr.isNull() )
	str.set( ethaddr.toString() );
    else if ( !wifiaddr.isNull() )
	str.set( wifiaddr.toString() );
    else if ( !otheraddr.isNull() )
	str.set(otheraddr.toString());
    else if ( !loopbackaddr.isNull() )
	str.set( loopbackaddr.toString() );

     return str.buf();
}


bool isLocalAddressInUse( const char* ipaddr )
{
    BufferString localaddr(ipaddr);
    if ( localaddr.isEmpty() )
	localaddr.set( localAddress() );

    const QList<QNetworkInterface> allif = QNetworkInterface::allInterfaces();
    for ( const auto& qni : allif )
    {
	if ( !qni.isValid() )
	    continue;

	const QNetworkInterface::InterfaceFlags flags = qni.flags();
	if ( !flags.testFlag(QNetworkInterface::IsUp) ||
	     !flags.testFlag(QNetworkInterface::IsRunning) )
	    continue;

	const QList<QNetworkAddressEntry> entries = qni.addressEntries();
	for ( const auto& ent : entries )
	{
	    const QHostAddress qaddr = ent.ip();
	    const BufferString addr( qaddr.toString() );
	    if ( addr == localaddr )
		return true;
	}
    }

    return false;
}


const char* hostName( const char* ip )
{
    mDeclStaticString( str );
    if ( StringView(ip) == localAddress() )
	return localHostName();

    const QHostInfo qhi = QHostInfo::fromName( QString(ip) );
    str = qhi.hostName();
    if ( str == ip )
	str.setEmpty();
    return str.buf();
}


const char* hostAddress( const char* hostname, bool ipv4only )
{
    mDeclStaticString( str );
    str.setEmpty();
    BufferString fqdn( hostname );
    if ( fqdn == localHostName() )
	fqdn.set( localFullHostName() );

    const QHostInfo qhi = QHostInfo::fromName( QString(fqdn) );
    const QList<QHostAddress> addresses = qhi.addresses();
    for ( const auto& addr : addresses )
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
    const bool isip = isValidIPAddress( host_ip );
    if ( msg )
    {
	msg->set( "For " )
	    .add( isip ? "IP address " : "host " ).add( host_ip ).add( ": " );
    }

    BufferString hostname, ipaddr;
    if ( isip )
    {
	hostname.set( hostName(host_ip) );
	if ( hostname.isEmpty() )
	{
	    if ( msg )
		msg->add( "Corresponding host name not found" );
	    return false;
	}
	if ( msg )
	    msg->add( "Found hostname: " ).add( hostname );
	ipaddr.set( host_ip );
    }
    else
    {
	ipaddr.set( hostAddress(host_ip) );
	if ( ipaddr.isEmpty() )
	{
	    if ( msg )
		msg->add( "Corresponding IP address not found" );
	    return false;
	}
	if ( msg )
	     msg->add( "Found IP address " ).add( ipaddr );
	hostname.set( host_ip );
    }

    const QHostInfo qhi = QHostInfo::fromName( QString(hostname) );
    if ( qhi.error() != QHostInfo::NoError )
    {
	if ( msg )
	    msg->add( qhi.errorString() );
	return false;
    }

    return true;
}


bool isValidIPAddress( const char* host_ip )
{
    QHostAddress addr;
    const QString qipaddr( host_ip );
    return addr.setAddress( qipaddr );
}


void macAddresses( BufferStringSet& names, BufferStringSet& addresses,
		   bool onlyactive )
{
    const QList<QNetworkInterface> allif = QNetworkInterface::allInterfaces();
    for ( const auto& qni : allif )
    {
	const QNetworkInterface::InterfaceFlags flags = qni.flags();
	if ( !qni.isValid() ||
	     !flags.testFlag(QNetworkInterface::CanBroadcast) ||
	     !flags.testFlag(QNetworkInterface::CanMulticast) )
	    continue;

	if ( onlyactive && ( !flags.testFlag(QNetworkInterface::IsUp) ||
			     !flags.testFlag(QNetworkInterface::IsRunning)) )
	    continue;

	if ( qni.name().isEmpty() || qni.hardwareAddress().isEmpty() )
	    continue;

	names.add( qni.name() );
	addresses.add( qni.hardwareAddress() );
    }
}


od_int64 bytesAvailable( const char* path )
{
    const QStorageInfo storageinfo( path );
    return storageinfo.bytesAvailable();
}


od_int64 bytesFree( const char* path )
{
    const QStorageInfo storageinfo( path );
    return storageinfo.bytesFree();
}


od_int64 bytesTotal( const char* path )
{
    const QStorageInfo storageinfo( path );
    return storageinfo.bytesTotal();
}


const char* fileSystemName( const char* path )
{
    mDeclStaticString( str );
    const QStorageInfo storageinfo( path );
    str = storageinfo.name();
    if ( str.isEmpty() )
	str = storageinfo.displayName();
    return str.buf();
}


const char* fileSystemType( const char* path )
{
    mDeclStaticString( str );
    const QStorageInfo storageinfo( path );
    str = storageinfo.fileSystemType().constData();
    return str.buf();
}


#define mToKbFac (1.0 / 1024.0)

int getFreeMBOnDisk( const char* path )
{
    const QStorageInfo storageinfo( path );
    if ( !storageinfo.isReady() || !storageinfo.isValid() )
	return mUdf(int);

    const od_int64 bytesavail = storageinfo.bytesAvailable();
    return (int)(bytesavail/1024/1024);
}


int getFreeMBOnDisk( const IOObj& ioobj )
{
    mDynamicCastGet(const IOStream*,iostrm,&ioobj)

    BufferString dir;
    if ( !iostrm )
	dir = GetDataDir();
    else
    {
	dir = FilePath( iostrm->fullUserExpr() ).pathOnly();
	if ( !File::exists(dir) )
	    dir = GetDataDir();
    }

    return getFreeMBOnDisk( dir );
}


void getFreeMBOnDiskMsg( int mb, uiString& str )
{
    str = od_static_tr( "getFreeMBOnDiskMsg",
			"Free space on disk: %1");
    if ( mIsUdf(mb) )
    {
	str.arg( "unknown" );
	return;
    }

    od_uint64 bytes = mb;
    bytes <<= 20;
    NrBytesToStringCreator converter( bytes );
    str.arg( converter.getString( bytes ) );
}


void getFreeMBOnDiskMsg( int mb, BufferString& bs )
{
    bs = "Free space on disk: ";
    if ( mIsUdf(mb) )
    {
	bs.add( "unknown" );
	return;
    }

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


bool getHostIDs( BufferStringSet& hostids, BufferString& errmsg )
{
    return OD::getHostIDs( hostids, errmsg );
}


const char* productName()
{
    mDeclStaticString( str );
    if ( str.isEmpty() )
    {
#ifdef __win__
	str.set( OD::Platform().osName() ).addSpace();
	BufferString winverstr( getWinVersion() );
	if ( winverstr.isEmpty() || winverstr.contains("Unknown") )
	    winverstr = getWinProductName();
	else
	{
	    winverstr.add( " Version " );
	    BufferString vernm( getWinDisplayName() );
	    if ( vernm.isEmpty() || vernm.contains("Unknown") )
		vernm.set( getWinEdition() );

	    winverstr.add( vernm.buf() );
	}

	str.add( winverstr.buf() );
#else
	str = QSysInfo::prettyProductName();
#endif
    }

    return str.buf();
}


const char* kernelVersion()
{
    mDeclStaticString( str );
    str = QSysInfo::kernelVersion();
    return str.buf();
}


const IOPar& graphicsInformation()
{
    static IOPar ret( "Graphics Information" );
    return ret;
}

} // namespace System
