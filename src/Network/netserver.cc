/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "netserver.h"

#include "commandlineparser.h"
#include "debug.h"
#include "envvars.h"
#include "genc.h"
#include "hostdata.h"
#include "netsocket.h"
#include "oscommand.h"
#include "separstr.h"
#include "systeminfo.h"

#include "qtcpservercomm.h"
#include <QHostAddress>
#include <QLocalServer>
#include <QString>


static Threads::Atomic<PortNr_Type> lastusableport_ = 0;

namespace Network
{

static const HostData* getLocalHostData( uiString& errmsg )
{
    static PtrMan<HostData> localhd_;
    static bool inited = false;
    if ( !inited )
    {
	inited = true;
	const HostDataList hdl( false );
	const HostData* localhd = hdl.localHost();
	if ( localhd && !localhd->connAddress().isEmpty() )
	{
	    localhd_ = new HostData( *localhd );
	    if ( !localhd_->isStaticIP() )
		localhd_->setIPAddress( System::localAddress() );

	    if ( !System::isLocalAddressInUse(localhd_->getIPAddress()) )
		localhd_ = nullptr;
	}

	/*TODO: invalidate localhd_ upon QNetworkSettingsManager
	*	currentWiredConnectionChanged,
	*	currentWifiConnectionChange,
		interfacesChanged		    */
    }

    return localhd_.ptr();
}

} // namespace Network


PortNr_Type Network::getNextCandidatePort()
{
    if ( lastusableport_ == 0 )
	lastusableport_ = (PortNr_Type)GetEnvVarIVal( "OD_START_PORT", 20049 );
    return lastusableport_ + 1;
}


PortNr_Type Network::getUsablePort( PortNr_Type portnr )
{
    uiRetVal uirv;
    return getUsablePort( uirv, portnr, 100 );
}


PortNr_Type Network::getUsablePort( uiRetVal& uirv, PortNr_Type portnr,
				    int nrtries )
{
    uiString errmsg;
    if ( portnr == 0 )
	portnr = getNextCandidatePort();

    for ( int idx=0; idx<nrtries; idx++, portnr++ )
    {
	if ( isPortFree(portnr,&errmsg) )
	    { lastusableport_ = portnr; return portnr; }
    }

    uirv = errmsg;
    return 0;
}


namespace Network {

static QHostAddress::SpecialAddress qtSpecAddr( SpecAddr specaddress )
{
    if ( specaddress == Any )
	return QHostAddress::Any;
    else if ( specaddress == IPv4 )
	return QHostAddress::AnyIPv4;
    else if ( specaddress == IPv6 )
	return QHostAddress::AnyIPv6;
    else if ( specaddress == Broadcast )
	return QHostAddress::Broadcast;
    else if ( specaddress == LocalIPv4 )
	return QHostAddress::LocalHost;
    else if ( specaddress == LocalIPv6 )
	return QHostAddress::LocalHostIPv6;

    return QHostAddress::Null;
}

static SpecAddr specAddrFromQt( QHostAddress qaddr )
{
    if ( qaddr == QHostAddress::Any )
	return Any;
    else if ( qaddr == QHostAddress::AnyIPv4 )
	return IPv4;
    else if ( qaddr == QHostAddress::AnyIPv6 )
	return IPv6;
    else if ( qaddr == QHostAddress::Broadcast )
	return Broadcast;
    else if ( qaddr == QHostAddress::LocalHost )
	return LocalIPv4;
    else if ( qaddr == QHostAddress::LocalHostIPv6 )
	return LocalIPv6;

    return None;
}

static QHostAddress anyQAddr()
{
    return QHostAddress( QHostAddress::Any );
}

static QHostAddress anyIPv4QAddr()
{
    return QHostAddress( QHostAddress::AnyIPv4 );
}

static QHostAddress anyIPv6QAddr()
{
    return QHostAddress( QHostAddress::AnyIPv6 );
}

static bool isAnyQAddr( const QHostAddress& qaddr )
{
    return qaddr.isEqual( anyQAddr() ) ||
	   qaddr.isEqual( anyIPv4QAddr() ) ||
	   qaddr.isEqual( anyIPv6QAddr() );
}

};


Network::Authority::Authority( const BufferString& servernm )
    : qhost_(*new QString)
    , qhostaddr_(*new QHostAddress)
{
    localFromString( servernm );
}


Network::Authority::Authority( const char* host, PortNr_Type port,
			       bool resolveipv6 )
    : qhost_(*new QString)
    , qhostaddr_(*new QHostAddress)
{
    setHost( host, resolveipv6 );
    setPort( port );
}


Network::Authority::Authority( const Authority& oth )
    : qhost_(*new QString)
    , qhostaddr_(*new QHostAddress)
{
    *this = oth;
}


Network::Authority::~Authority()
{
    delete &qhost_;
    delete &qhostaddr_;
}


Network::Authority& Network::Authority::operator=( const Authority& oth )
{
    if ( &oth == this )
	return *this;

    userinfo_ = oth.userinfo_;
    port_ = oth.port_;
    qhost_ = oth.qhost_;
    qhostaddr_ = oth.qhostaddr_;
    hostisaddress_ = oth.hostisaddress_;
    servernm_ = oth.servernm_;

    return *this;
}


bool Network::Authority::operator==( const Authority& oth ) const
{
    if ( &oth == this )
	return true;

    return port_ == oth.port_ &&
	   qhostaddr_ == oth.qhostaddr_ &&
	   qhost_ == oth.qhost_ &&
	   hostisaddress_ == oth.hostisaddress_ &&
	   userinfo_ == oth.userinfo_ &&
	    servernm_ == oth.servernm_;
}


BufferString Network::Authority::getServerName() const
{
    return servernm_;
}


Network::SpecAddr Network::Authority::serverAddress() const
{
    if ( isLocal() )
	return None;

    return specAddrFromQt( qhostaddr_ );
}


bool Network::Authority::isOK() const
{
    return isLocal() || (addressIsValid() && hasAssignedPort());
}


bool Network::Authority::isUsable() const
{
    if ( isLocal() )
	return true;

    bool ret = hasAssignedPort();
#ifdef __win__
    if ( ret )
        ret = isPortFree( port_ );
#endif
    return ret;
}


bool Network::Authority::addressIsValid() const
{
    return !qhostaddr_.isNull();
}


bool Network::Authority::portIsFree( uiString* errmsg ) const
{
    if ( !hasAssignedPort() )
    {
	if ( errmsg )
	    *errmsg = tr("Network port is not assigned.");
	return false;
    }

    return isPortFree( port_, errmsg );
}


void Network::Authority::fromString( const char* str, bool resolveipv6 )
{
    StringView hoststr( str );
    if ( hoststr.contains('@') )
    {
	const SeparString usersep( hoststr, '@' );
	userinfo_.set( usersep[0] );
	hoststr = usersep.size() > 1 ? hoststr.find('@')+1 : nullptr;
    }
    else
	userinfo_.setEmpty();

    if ( hoststr.contains('[') )
    {
	BufferString hostnm( hoststr );
	hoststr = hostnm.find(']') + 1;
	hostnm.replace( '[', ' ' ).replace( ']', '\0' );
	hostnm.trimBlanks();
	setHost( hostnm );
	if ( hoststr.contains(':') )
	{
	    const SeparString hostsep( hoststr, ':' );
	    setPort( hostsep.size() > 1 ?
		     mCast(PortNr_Type, hostsep[1].toInt()) : 0 );
	}
	else
	    setPort( 0 );
    }
    else
    {
	if ( hoststr.contains(':') )
	{
	    const SeparString hostsep( hoststr, ':' );
	    setHost( hostsep[0], resolveipv6 );
	    setPort( hostsep.size() > 1 ?
		 mCast(PortNr_Type, hostsep[1].toInt()) : 0 );
	}
	else
	{
	    setHost( hoststr, resolveipv6 );
	    setPort( 0 );
	}
    }
}


Network::Authority& Network::Authority::localFromString( const char* str )
{
    servernm_ = str;
    qhostaddr_.setAddress( QHostAddress::Null );
    port_ = 0;
    return *this;
}


BufferString Network::Authority::toString() const
{
    BufferString ret( userinfo_ );
    if ( !ret.isEmpty() )
	ret.add( "@" );
    ret.add( getHost() );
    if ( port_ > 0 )
	ret.add( ":" ).add( port_ );
    return ret;
}


BufferString Network::Authority::getHost() const
{
    if ( isLocal() )
	return getServerName();

    if ( !addressIsValid() )
	return BufferString::empty();

    if ( qhostaddr_.isLoopback() )
	return Socket::sKeyLocalHost();

    const BufferString qhostaddrstr( qhostaddr_.toString() );
    BufferString ret;
    if ( hostisaddress_ )
    {
	if ( isAnyQAddr(qhostaddr_) )
	    return System::localFullHostName();

	const QAbstractSocket::NetworkLayerProtocol protocol =
							qhostaddr_.protocol();
	const bool ipv6 = protocol == QAbstractSocket::IPv6Protocol;
	if ( ipv6 )
	    ret.add( "[" );
	ret.add( qhostaddr_.toString() );
	if ( ipv6 )
	    ret.add( "]" );
    }
    else
    {
	const BufferString qhoststr( qhost_ );
	const BufferString hostnm( System::hostAddress(qhoststr) );
	if ( !hostnm.isEmpty() )
	    ret.set( qhost_ );
    }

    return ret;
}


BufferString Network::Authority::getConnHost( ConnType typ ) const
{
    if ( isLocal() || !addressIsValid() || qhostaddr_.isLoopback() ||
	 ((hostisaddress_ && !isAnyQAddr(qhostaddr_)) || !hostisaddress_) )
	return BufferString::empty();

    static uiString errmsg;
    const HostData* localhd = getLocalHostData( errmsg );
    if ( !localhd && DBG::isOn(DBG_MM) )
	DBG::message( DBG_MM, ::toString(errmsg) );

    BufferString ret;
    switch ( typ )
    {
	case FQDN:
	    ret.set( localhd ? localhd->getHostName( true )
			     : System::localFullHostName() ); break;
	case HostName:
	    ret.set( localhd ? localhd->getHostName( false )
			     : System::localHostName() ); break;
	case IPv4Address:
	    ret.set( localhd ? localhd->getIPAddress()
			     : System::localAddress() ); break;
	default: break;
    }

    return ret;
}


Network::Authority& Network::Authority::setUserInfo( const char* inf )
{ userinfo_ = inf; return *this; }


Network::Authority& Network::Authority::setHost( const char* nm,
                                        bool resolveipv6 )
{
    setHostAddress( nm, resolveipv6 );
    return *this;
}


Network::Authority& Network::Authority::setPort( PortNr_Type port )
{ port_ = port; return *this; }


void Network::Authority::setFreePort( uiRetVal& uirv )
{
    setPort( getUsablePort(uirv) );
}


void Network::Authority::setHostAddress( const char* host, bool resolveipv6 )
{
    BufferString hostnm( host );
    hostnm.replace( '[', ' ' ).replace( ']', ' ' );
    hostnm.trimBlanks();

    qhost_ = hostnm.buf();
    if ( hostnm.isEmpty() )
	qhostaddr_.setAddress( QHostAddress::Null );
    else
	qhostaddr_.setAddress( qhost_ );

    hostisaddress_ = qhostaddr_.protocol() !=
		     QAbstractSocket::UnknownNetworkLayerProtocol;

    if ( !hostisaddress_ )
    {
	qhostaddr_.setAddress(
			QString(System::hostAddress(hostnm,!resolveipv6)) );
    }

    if ( qhostaddr_.isNull() )
	qhostaddr_.clear();
}


void Network::Authority::addTo( OS::MachineCommand& mc, const char* ky ) const
{
    if ( ky && *ky )
    {
        mc.addKeyedArg( ky, toString() )
          .addFlag( Server::sKeyLocal() );
        return;
    }

    mc.addKeyedArg( Server::sKeyHostName(), getHost() );
    if ( isLocal() )
        mc.addFlag( Server::sKeyLocal() );
    else
        mc.addKeyedArg( Server::sKeyPort(), getPort() );
}


Network::Authority& Network::Authority::setFrom(const CommandLineParser& parser,
			    const char* defservernm,
			    const char* defhostnm, PortNr_Type defport )
{
    CommandLineParser& eparser = const_cast<CommandLineParser&>( parser );
    eparser.setKeyHasValue( Server::sKeyHostName() );
    eparser.setKeyHasValue( Server::sKeyPort() );

    const bool localtcp = parser.hasKey( Server::sKeyLocal() );
    if ( localtcp )
    {
        BufferString servernm( defservernm );
	if ( !parser.getVal(Server::sKeyHostName(),servernm) )
	    servernm = getAppServerName( servernm );
        return localFromString( servernm );
    }
    else
    {
        BufferString hostnm( defhostnm );
        parser.getVal( Server::sKeyHostName(), hostnm);
        PortNr_Type port = defport;
        int portint;
        if ( parser.getVal(Server::sKeyPort(),portint) )
            port = mCast(PortNr_Type,portint);
        port = Network::getUsablePort( port );
        return setHost( hostnm, false ).setPort( port );
    }
}


BufferString Network::Authority::getAppServerName( const char* nm )
{
    BufferString ret(nm);
    if ( ret.isEmpty() )
        ret.set("opendtect");
    ret.add( ":" ).add( GetPID() );
    return ret;
}


static int sockid = 0;

static int getNewID()
{
    return ++sockid;
}


Network::Server::Server( bool islocal )
    : newConnection(this)
    , readyRead(this)
{
    if ( islocal )
    {
	qlocalserver_ = new QLocalServer();
	comm_ = new QTcpServerComm( qlocalserver_, this );
    }
    else
    {
	qtcpserver_ = new QTcpServer();
	comm_ = new QTcpServerComm( qtcpserver_, this );
    }
}


Network::Server::~Server()
{
    detachAllNotifiers();
    if ( isListening() )
	close();
    delete qtcpserver_;
    delete qlocalserver_;
    delete comm_;
    deepErase( sockets2bdeleted_ );
}


bool Network::Server::listen( SpecAddr specaddress, PortNr_Type prt )
{
    if ( qtcpserver_ )
    {
#ifdef __win__
        const bool wasportfree = isPortFree(prt);
#else
        const bool wasportfree = true;
#endif
        const bool ret = wasportfree &&
            qtcpserver_->listen(QHostAddress(qtSpecAddr(specaddress)), prt);
        if ( !ret )
        {
            errmsg_.set("Cannot list to port ").add(prt);
            close();
        }
        return ret;
    }

    if ( isLocal() )
    {
	pErrMsg("Wrong listen function called on local server");
	return false;
    }

    DBG::forceCrash(false);
    return false;
}


bool Network::Server::listen( const char* servernm, uiRetVal& ret )
{
    if ( qlocalserver_ )
    {
	const bool canlisten = qlocalserver_->listen( servernm );
	if ( !canlisten )
	{
	    ret.set(tr("%1 server name is already in use").arg(servernm));
	    close();
	}
	return canlisten;
    }

    if ( !isLocal() )
    {
	pErrMsg( "Wrong listen function called on TCP-IP server" );
	return false;
    }

    DBG::forceCrash(false);
    return false;
}


bool Network::Server::isListening() const
{
    if ( isLocal() )
	return qlocalserver_->isListening();

    return qtcpserver_->isListening();
}

PortNr_Type Network::Server::port() const
{
    if ( isLocal() )
    {
	pErrMsg("Local server does not have a port");
	return 0;
    }
    return qtcpserver_->serverPort();
}


Network::Authority Network::Server::authority() const
{
    Authority ret;
    if ( isLocal() )
    {
        const BufferString servernm( qlocalserver_->serverName() );
        ret.localFromString( servernm );
	return ret;
    }

    const QHostAddress qaddr = qtcpserver_->serverAddress();
    BufferString addr;
    if ( qaddr.protocol() != QAbstractSocket::UnknownNetworkLayerProtocol )
	addr.set( qaddr.toString() );

    return ret.setHost( addr ).setPort( port() );
}


void Network::Server::close()
{
    if ( isLocal() )
	qlocalserver_->close();
    else
	qtcpserver_->close();
}


const char* Network::Server::errorMsg() const
{
    if ( isLocal() )
	errmsg_ = qlocalserver_->errorString().toLatin1().constData();
    else
	errmsg_ = qtcpserver_->errorString().toLatin1().constData();
    return errmsg_.buf();
}


bool Network::Server::hasPendingConnections() const
{
    if ( isLocal() )
	return qlocalserver_->hasPendingConnections();

    return qtcpserver_->hasPendingConnections();
}

QTcpSocket* Network::Server::nextPendingConnection()
{
    return qtcpserver_->nextPendingConnection();
}


QLocalSocket* Network::Server::nextPendingLocalConnection()
{
    return qlocalserver_->nextPendingConnection();
}


void Network::Server::notifyNewConnection()
{
    if ( !hasPendingConnections() )
	return;

    Socket* socket = nullptr;
    if ( isLocal() )
	socket = new Socket( nextPendingLocalConnection() );
    else
	socket = new Socket( nextPendingConnection() );

    socket->readyRead.notify( mCB(this,Server,readyReadCB) );
    mAttachCB( socket->disconnected, Server::disconnectCB );
    sockets_ += socket;
    const int id = getNewID();
    ids_ += id;

    newConnection.trigger( id );
}


void Network::Server::readyReadCB( CallBacker* cb )
{
    mDynamicCastGet(Socket*,socket,cb);
    if ( !socket ) return;

    const int idx = sockets_.indexOf( socket );
    if ( idx<0 )
	return;

    readyRead.trigger( ids_[idx] );
}


void Network::Server::disconnectCB( CallBacker* cb )
{
    mDynamicCastGet(Socket*,socket,cb);
    if ( !socket ) return;

    //socket->readyRead.remove( mCB(this,Server,readyReadCB) );
    const int idx = sockets_.indexOf( socket );
    if ( idx>=0 )
    {
	sockets_.removeSingle( idx );
	ids_.removeSingle( idx );
    }

    const int delidx = sockets2bdeleted_.indexOf( socket );
    if ( delidx < 0 )
	sockets2bdeleted_ += socket;
}


void Network::Server::read( int id, BufferString& data ) const
{
    const Socket* socket = getSocket( id );
    if ( !socket )
	return;

    socket->read( data );
}


void Network::Server::read( int id, IOPar& par ) const
{
    const Socket* socket = getSocket( id );
    if ( !socket )
	return;

    socket->read( par );
}


int Network::Server::write( int id, const IOPar& par )
{
    Socket* socket = getSocket( id );
    return socket ? socket->write( par ) : 0;
}


int Network::Server::write( int id, const char* str )
{
    Socket* socket = getSocket( id );
    return socket ? socket->write( StringView(str) ) : 0;
}


Network::Socket* Network::Server::getSocket( int id )
{
    const int idx = ids_.indexOf( id );
    if ( idx<0 )
	return 0;

    return sockets_[idx];
}


const Network::Socket* Network::Server::getSocket( int id ) const
{ return const_cast<Server*>( this )->getSocket( id ); }


bool Network::Server::waitForNewConnection( int msec )
{
    return qtcpserver_->waitForNewConnection( msec );
}
