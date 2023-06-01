/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "applicationdata.h"
#include "hostdata.h"
#include "moddepmgr.h"
#include "netreqpacket.h"
#include "netreqconnection.h"
#include "netserver.h"
#include "netsocket.h"
#include "testprog.h"
#include "timer.h"

#ifdef __win__
# include "time.h"
#endif

#include <QHostAddress>
#include <QString>
#include <QTcpSocket>

static const char* portkey = Network::Server::sKeyPort();
static const char* tcp6key = "tcp6";
static const char* tcp4key = "tcp4";
static const char* localstr = Network::Server::sKeyLocal();

static void printBatchUsage()
{
    od_ostream& strm = logStream();
    strm << "Usage: " << "test_listening_server [-h] "
			 "[--port PORT] [--tcp6] [--tcp4] [--local]\n\n";
    strm << "Server application with listening socket\n\n";
    strm << "optional arguments:\n";
    strm << "  -h, --help\t\tshow this help message and exit\n\n";
    strm << "  --" << portkey << " PORT\t\tPort to listen"
				 " on (default: 37504)\n";
    strm << "  --" << tcp6key << "\t\tlisten only on IPv6 interfaces\n";
    strm << "  --" << tcp4key << "\t\tlisten only on IPv4 interfaces\n";
    strm << "  --" << localstr << "\t\tlisten on localhost only";
    strm << od_endl;
}


namespace Network
{

class RequestEchoServer : public CallBacker
{
public:
    RequestEchoServer( PortNr_Type port, SpecAddr addr, unsigned short timeout )
	: server_(port,addr)
	, timeout_( timeout )
    {
	mAttachCB( server_.newConnection, RequestEchoServer::newConnectionCB );
	mAttachCB( timer_.tick, RequestEchoServer::timerTick );

	lastactivity_ = time( 0 );
	timer_.start( 1000, false );
    }

    ~RequestEchoServer()
    {
	detachAllNotifiers();
	CallBack::removeFromThreadCalls( this );
	deepErase( conns_ );
    }

private:

    void newConnectionCB( CallBacker* )
    {
	lastactivity_ = time( 0 );
	RequestConnection* newconn = server_.pickupNewConnection();
	if ( !newconn )
	    return;

	Socket* socket = newconn->socket();
	if ( socket && socket->isConnected() )
	{
	    mDynamicCastGet(const QTcpSocket*,qsocket,socket->qSocket());
	    if ( qsocket )
	    {
		const QHostAddress qaddr = qsocket->peerAddress();
		const quint16 qport = qsocket->peerPort();
		QString scopeid = qaddr.scopeId();
		if ( scopeid.isEmpty() )
		    scopeid.append( "0" );
		logStream() << "Connected by ('"
			    << BufferString(qaddr.toString()) << "', "
			    << qport << ", "
			    << 0 << ", " << BufferString(scopeid)
			    << ")" << od_newline;
	    }
	}

	mAttachCB( newconn->packetArrived, RequestEchoServer::packetArrivedCB );
	mAttachCB( newconn->connectionClosed, RequestEchoServer::connClosedCB );

	conns_.add( newconn );
    }

    void packetArrivedCB( CallBacker* cb )
    {
	lastactivity_ = time( 0 );
	mCBCapsuleUnpackWithCaller( od_int32, reqid, cber, cb );

	RequestConnection* conn = static_cast<RequestConnection*>( cber );

	RefMan<RequestPacket> packet = conn->pickupPacket( reqid, 200 );
	if ( !packet )
	{
	    packet = conn->getNextExternalPacket();
	    if ( !packet )
		return;
	}

	BufferString packetstring;
	packet->getStringPayload( packetstring );
	if ( packetstring==Server::sKeyKillword() )
	{
	    conn->socket()->disconnectFromHost();
	    logStream() << "Kill requested " << od_endl;
	    CallBack::addToMainThread(
			mCB(this,RequestEchoServer,closeServerCB));
	}
	else if ( packetstring=="Disconnect" )
	{
	    conn->socket()->disconnectFromHost();
	}
	else
	{
	    conn->sendPacket( *packet );
	    packet = nullptr;
	}
    }

    void connClosedCB( CallBacker* )
    {
	CallBack::addToMainThread(
			mCB(this,RequestEchoServer,cleanupOldConnections));
    }

    void cleanupOldConnections( CallBacker* )
    {
	for ( int idx=0; idx<conns_.size(); idx++ )
	{
	    if ( !conns_[idx]->isOK() )
	    {
		delete conns_.removeSingle( idx );
		idx--;
	    }
	}
    }

    void closeServerCB( CallBacker* )
    {
	deepErase( conns_ );
	ApplicationData::exit( exitstatus_ );
    }

    void timerTick( CallBacker* )
    {
	const time_t curtime = time( 0 );
	if ( curtime-lastactivity_>timeout_ )
	{
	    logStream() << "Timeout reached, exiting" << od_endl;
	    CallBack::addToMainThread(
			mCB(this,RequestEchoServer,closeServerCB));
	}

	if ( server_.isOK() && !inited_ )
	{
	    const Authority auth = server_.getAuthority();
	    tstStream() << "Server listening with authority "
		    << auth.toString() << od_newline;
	    tstStream() << "Server listening with connection authority "
		    << auth.getConnHost( Authority::IPv4 ) << od_endl;
	    uiStringSet msgs;
	    const HostDataList hdl( false );
	    const bool res = hdl.isOK( msgs, true );
	    if ( !res )
	    {
		errStream() << "Configuration error: "
		    << toString( msgs.cat() ) << od_endl;
		exitstatus_ = 1;
		CallBack::addToMainThread(
		    mCB(this,RequestEchoServer,closeServerCB) );
	    }
	}
	else if ( !server_.isOK() )
	{
	    errStream() << "Server error: "
		        << toString(server_.errMsg()) << od_endl;
	    exitstatus_ = 2;
	    CallBack::addToMainThread(
			mCB(this,RequestEchoServer,closeServerCB) );
	}

	inited_ = true;
    }


    RequestServer			server_;
    bool				inited_ = false;
    Timer				timer_;
    time_t				lastactivity_;
    time_t				timeout_;
    int					exitstatus_ = 0;
    ObjectSet<RequestConnection>	conns_;
};

} // namespace Network


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();
    ApplicationData app;
    OD::ModDeps().ensureLoaded( "Network" );

    CommandLineParser& parser = clParser();
    parser.setKeyHasValue( portkey );
    parser.setKeyHasValue( Network::Server::sKeyTimeout() );

    if ( parser.hasKey("help") || parser.hasKey("h") )
    {
	printBatchUsage();
	return 1;
    }

    PortNr_Type port = 37504;
    parser.getVal( portkey, port );
    bool onlytcp6 = parser.hasKey( tcp6key );
    const bool onlytcp4 = parser.hasKey( tcp4key );
    const bool uselocal = parser.hasKey( localstr );
    if ( uselocal && !onlytcp6 && !onlytcp4 )
	onlytcp6 = true;

    Network::SpecAddr addr = Network::None;
    if ( uselocal )
	addr = onlytcp4 ? Network::LocalIPv4 : Network::LocalIPv6;
    else
    {
	addr = onlytcp6 ? Network::IPv6
			: (onlytcp4 ? Network::IPv4 : Network::Any);
    }

    od_ostream& strm = logStream();
    if ( uselocal )
	strm << "Listening on localhost only" << od_newline;

    if ( onlytcp6 )
	strm << "Listening on only IPv6 interfaces";
    else if ( onlytcp4 )
	strm << "Listening on only IPv4 interfaces";
    else
	strm << "Listening on both IPv4 and IPv6 interfaces";

    unsigned short timeout = sCast(unsigned short,600); //sec
    parser.getVal( Network::Server::sKeyTimeout(), timeout );
    strm << " with a " << timeout << " seconds timeout" << od_newline;
    strm << "Listening server using port: " << port << od_endl;

    Network::RequestEchoServer tester( port, addr, timeout );

    return app.exec();
}
