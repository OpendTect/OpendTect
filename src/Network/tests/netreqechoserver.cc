/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2013
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "netreqconnection.h"

#include "applicationdata.h"
#include "netreqpacket.h"
#include "manobjectset.h"
#include "netserver.h"
#include "netsocket.h"
#include "timer.h"

#include "ptrman.h"
#include "testprog.h"

#include <time.h>

namespace Network
{

class RequestEchoServer : public CallBacker
{
public:
    RequestEchoServer( unsigned short port, int timeout, ApplicationData& app )
	: server_( new RequestServer(port) )
	, app_( app )
	, timeout_( timeout )
    {
	mAttachCB( server_->newConnection,
		  RequestEchoServer::newConnectionCB );

	Threads::sleep( 1 );
	if ( !server_->isOK() )
	    closeServerCB( 0 );

	lastactivity_ = time( 0 );
	mAttachCB( timer_.tick, RequestEchoServer::timerTick );
	timer_.start( 1000, false );
    }


    ~RequestEchoServer()
    {
	detachAllNotifiers();
    }


    void newConnectionCB( CallBacker* )
    {
	lastactivity_ = time( 0 );
	RequestConnection* newconn = server_->pickupNewConnection();
	if ( !newconn )
	    return;

	if ( !quiet )
	    od_cout() << "New connection " << newconn->ID()
		      << " on port " << server_->server()->port() << od_endl;

	mAttachCB( newconn->packetArrived, RequestEchoServer::packetArrivedCB );
	mAttachCB( newconn->connectionClosed, RequestEchoServer::connClosedCB );

	conns_ += newconn;
    }


    void packetArrivedCB( CallBacker* cb )
    {
	lastactivity_ = time( 0 );
	mCBCapsuleUnpackWithCaller( od_int32, reqid, cber, cb );

	RequestConnection* conn = static_cast<RequestConnection*>( cber );

	PtrMan<RequestPacket> packet = conn->pickupPacket( reqid, 200 );
	if ( !packet )
	{
	    packet = conn->getNextExternalPacket();
	    if ( !packet )
		return;
	}

	if ( !quiet )
	    od_cout() << "Request " << packet->requestID()
		  << " received packet "
	          << packet->subID() << " size " << packet->payloadSize()
		  << od_endl;


	BufferString packetstring;

	packet->getStringPayload( packetstring );
	if ( packetstring=="Kill" )
	{
	    if ( !quiet )
		od_cout() << "Kill requested " << od_endl;

	    CallBack::addToMainThread(
			mCB(this,RequestEchoServer,closeServerCB));
	}
	else if ( packetstring=="Disconnect" )
	{
	    conn->socket()->disconnectFromHost();
	}
	else if ( packetstring=="New" )
	{
	    Network::RequestPacket newpacket;
	    BufferString sentmessage = "The answer is 42";
	    newpacket.setIsNewRequest();
	    newpacket.setStringPayload( sentmessage );
	    conn->sendPacket( newpacket );
	}
	else
	{
	    conn->sendPacket( *packet );
	    od_cout() << "Request " << packet->requestID()
		  << " sent packet "
		  << packet->subID() << " size " << packet->payloadSize()
		  << od_endl;
	    packet.release();
	}
    }

    void connClosedCB( CallBacker* cb )
    {
	RequestConnection* conn = (RequestConnection*) cb;
	if ( !quiet )
	    od_cout() << "Connection " << conn->ID() << " closed." << od_endl;
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
	conns_.erase();
	ApplicationData::exit( 0 );
    }

    void timerTick( CallBacker* )
    {
	const time_t now = time( 0 );
	if ( now-lastactivity_>timeout_ )
	{
	    if ( !quiet )
		od_cout() << "Timeout" << od_endl;

	    CallBack::addToMainThread(
			mCB(this,RequestEchoServer,closeServerCB));
	}
    }


    Timer				timer_;
    time_t				lastactivity_;
    time_t				timeout_;
    ApplicationData&			app_;
    ManagedObjectSet<RequestConnection> conns_;
    RequestServer*			server_;
};

} //Namespace


int main(int argc, char** argv)
{
    mInitTestProg();

    //Make standard test-runs just work fine.
    if ( !clparser.hasKey("port") )
	ExitProgram( 0 );

    ApplicationData app;

    int startport = 1025;
    clparser.getVal( "port", startport );

    int timeout = 120;
    clparser.getVal( "timeout", timeout );

    Network::RequestEchoServer server( mCast(unsigned short,startport),
					     timeout, app );

    ExitProgram( app.exec() );
}
