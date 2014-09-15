/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2013
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "tcpsocket.h"

#include "netreqcommunic.h"

#include "netreqpacket.h"
#include "applicationdata.h"
#include "ptrman.h"
#include "testprog.h"


namespace Network
{

class RequestCommunicatorServer : public CallBacker
{
public:
    RequestCommunicatorServer( short port, ApplicationData& app )
	: comm_( new RequestCommunicator(port) )
	, app_( app )
    {
	mAttachCB( comm_->packetArrived,
		  RequestCommunicatorServer::packetArrivedCB );

	Threads::sleep( 1 );
	if ( !comm_->isOK() )
	    closeServerCB( 0 );


    }

    void packetArrivedCB( CallBacker* cb )
    {
	mCBCapsuleUnpack( od_int32, reqid, cb );
	PtrMan<RequestPacket> packet = comm_->pickupPacket( reqid, 200 );
	if ( !packet )
	{
	    packet = comm_->getNextExternalPacket();
	    if ( !packet )
		return;
	}

	BufferString packetstring;

	packet->getStringPayload( packetstring );
	if ( packetstring=="Kill" )
	{
	    app_.addToEventLoop(
			mCB(this,RequestCommunicatorServer,closeServerCB));
	}
	else if ( packetstring=="New" )
	{
	    Network::RequestPacket newpacket;
	    BufferString sentmessage = "The answer is 42";
	    newpacket.setIsNewRequest();
	    newpacket.setStringPayload( sentmessage );
	    comm_->sendPacket( newpacket );
	}
	else
	{
	    comm_->sendPacket( *packet.release() );
	}
    }

    void closeServerCB( CallBacker* )
    {
	deleteAndZeroPtr( comm_ );
	ApplicationData::exit( 0 );
    }


    ApplicationData&		app_;
    RequestCommunicator*	comm_;
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

    Network::RequestCommunicatorServer server( startport, app );

    ExitProgram( app.exec() );
}
