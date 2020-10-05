/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2013
-*/



#include "applicationdata.h"
#include "netreqpacket.h"
#include "manobjectset.h"
#include "netreqconnection.h"
#include "netserver.h"
#include "netsocket.h"
#include "ptrman.h"
#include "testprog.h"
#include "timer.h"

#include <time.h>

namespace Network
{

class RequestEchoServer : public CallBacker
{
public:
    RequestEchoServer( PortNr_Type port, unsigned short timeout )
	: server_(port)
	, timeout_( timeout )
    {
	mAttachCB( server_.newConnection, RequestEchoServer::newConnectionCB );

	Threads::sleep( 1 );
	if ( !server_.isOK() )
	    closeServerCB( 0 );

	mAttachCB( timer_.tick, RequestEchoServer::timerTick );

	lastactivity_ = time( 0 );
	timer_.start( 1000, false );
    }


    ~RequestEchoServer()
    {
	detachAllNotifiers();
	CallBack::removeFromThreadCalls( this );
    }


    void newConnectionCB( CallBacker* )
    {
	lastactivity_ = time( 0 );
	RequestConnection* newconn = server_.pickupNewConnection();
	if ( !newconn )
	    return;

	logStream() << "New connection " << newconn->ID()
		      << " on port " << server_.server()->port() << od_endl;

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
	    od_cout() << "Request " << packet->requestID()
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
    }


    RequestServer			server_;
    Timer				timer_;
    time_t				lastactivity_;
    time_t				timeout_;
    ManagedObjectSet<RequestConnection> conns_;
};

} //Namespace


int mTestMainFnName(int argc, char** argv)
{
    mInitTestProg();

    //Make standard test-runs just work fine.
    if ( !clParser().hasKey(Network::Server::sKeyPort()) )
	return 0;
    clParser().setKeyHasValue( Network::Server::sKeyPort() );
    clParser().setKeyHasValue( Network::Server::sKeyTimeout() );

    ApplicationData app;

    int startport = 1025;
    clParser().getKeyedInfo( Network::Server::sKeyPort(), startport );
    int timeout = 120;
    clParser().getKeyedInfo( Network::Server::sKeyTimeout(), timeout );

    Network::RequestEchoServer server( mCast(PortNr_Type,startport),
				       mCast(unsigned short,timeout) );

    logStream() << "Listening to port " << server.server_.server()->port()
		  << " with a " << server.timeout_ << " second timeout\n";

    return app.exec();
}
