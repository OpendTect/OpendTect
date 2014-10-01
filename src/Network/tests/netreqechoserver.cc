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

#include "ptrman.h"
#include "testprog.h"


namespace Network
{

class RequestEchoServer : public CallBacker
{
public:
    RequestEchoServer( unsigned short port, ApplicationData& app )
	: server_( new RequestServer(port) )
	, app_( app )
    {
	mAttachCB( server_->newConnection,
		  RequestEchoServer::newConnectionCB );

	Threads::sleep( 1 );
	if ( !server_->isOK() )
	    closeServerCB( 0 );


    }

    void newConnectionCB( CallBacker* )
    {
	RequestConnection* newconn = server_->pickupNewConnection();
	if ( !newconn )
	    return;

	mAttachCB( newconn->packetArrived, RequestEchoServer::packetArrivedCB );
	mAttachCB( newconn->connectionClosed, RequestEchoServer::connClosedCB );

	conns_ += newconn;
    }


    void packetArrivedCB( CallBacker* cb )
    {
	mCBCapsuleUnpackWithCaller( od_int32, reqid, cber, cb );

	RequestConnection* conn = static_cast<RequestConnection*>( cber );

	PtrMan<RequestPacket> packet = conn->pickupPacket( reqid, 200 );
	if ( !packet )
	{
	    packet = conn->getNextExternalPacket();
	    if ( !packet )
		return;
	}

	BufferString packetstring;

	packet->getStringPayload( packetstring );
	if ( packetstring=="Kill" )
	{
	    app_.addToEventLoop(
			mCB(this,RequestEchoServer,closeServerCB));
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
	    conn->sendPacket( *packet.release() );
	}
    }

    void connClosedCB( CallBacker* cb )
    {
	app_.addToEventLoop( mCB(this,RequestEchoServer,closeServerCB));
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

    Network::RequestEchoServer server( mCast(unsigned short,startport), app );

    ExitProgram( app.exec() );
}
