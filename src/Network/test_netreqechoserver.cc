/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "applicationdata.h"
#include "netreqpacket.h"
#include "manobjectset.h"
#include "netreqconnection.h"
#include "netserver.h"
#include "netsocket.h"
#include "testprog.h"
#include "timer.h"

#ifdef __win__
# include "time.h"
#endif


namespace Network
{

class RequestEchoServer : public CallBacker
{
public:
    RequestEchoServer( const Network::Authority& auth, unsigned short timeout )
	: server_(auth)
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


    void newConnectionCB( CallBacker* )
    {
	lastactivity_ = time( 0 );
	RequestConnection* newconn = server_.pickupNewConnection();
	if ( !newconn )
	    return;

	if ( server_.server()->isLocal() )
	{
	    logStream() << "New connection " << newconn->ID()
			<< " on local server: "
			<< server_.server()->authority().getServerName()
			<< od_endl;
	}
	else
	{
	    logStream() << "New connection " << newconn->ID()
			<< " on port " << server_.server()->port() << od_endl;
	}

	mAttachCB( newconn->packetArrived, RequestEchoServer::packetArrivedCB );
	mAttachCB( newconn->connectionClosed, RequestEchoServer::connClosedCB );

	conns_ += newconn;
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

	logStream() << "Request " << packet->requestID()
		    << " received packet "
		    << packet->subID() << " size " << packet->payloadSize()
		    << od_endl;

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
	else if ( packetstring=="New" )
	{
	    RefMan<RequestPacket> newpacket = new RequestPacket;
	    BufferString sentmessage = "The answer is 42";
	    newpacket->setIsNewRequest();
	    newpacket->setStringPayload( sentmessage );
	    conn->sendPacket( *newpacket );
	}
	else
	{
	    conn->sendPacket( *packet );
	    logStream() << "Request " << packet->requestID()
		      << " sent packet "
		      << packet->subID() << " size " << packet->payloadSize()
		      << od_endl;
	    packet = nullptr;
	}
    }

    void connClosedCB( CallBacker* cb )
    {
	RequestConnection* conn = (RequestConnection*) cb;
	logStream() << "Connection " << conn->ID() << " closed." << od_endl;
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
	conns_.setEmpty();
	ApplicationData::exit( 0 );
    }

    void timerTick( CallBacker* )
    {
	const time_t curtime = time( 0 );
	if ( curtime-lastactivity_>timeout_ )
	{
	    logStream() << "Timeout" << od_endl;
	    CallBack::addToMainThread(
			mCB(this,RequestEchoServer,closeServerCB));
	}

	if ( !server_.isOK() )
	{
	    errStream() << "Server error: "
		        << toString(server_.errMsg()) << od_endl;
	    CallBack::addToMainThread(
			mCB(this,RequestEchoServer,closeServerCB) );
	}
    }


    RequestServer			server_;
    Timer				timer_;
    time_t				lastactivity_;
    time_t				timeout_;
    ObjectSet<RequestConnection>	conns_;
};

} // namespace Network


int mTestMainFnName(int argc, char** argv)
{
    mInitTestProg();

    ApplicationData app;

    Network::Authority auth;
    auth.setFrom( clParser(), "test_netreq",
		  Network::Socket::sKeyLocalHost(), PortNr_Type(1025) );
    if ( !auth.isUsable() )
    {
	od_ostream& strm = errStream();
	strm << "Incorrect authority '" << auth.toString() << "'";
	strm << "for starting the server" << od_endl;
	return 1;
    }

    int timeout = 600;
    clParser().setKeyHasValue( Network::Server::sKeyTimeout() );
    clParser().getVal( Network::Server::sKeyTimeout(), timeout );

    Network::RequestEchoServer tester( auth, mCast(unsigned short,timeout) );

    logStream() << "Listening to " << auth.toString()
		<< " with a " << tester.timeout_ << " second timeout\n";

    return app.exec();
}
