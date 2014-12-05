/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2013
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "tcpsocket.h"

#include "applicationdata.h"
#include "oscommand.h"
#include "ptrman.h"
#include "testprog.h"

#include "netreqconnection.h"
#include "netreqpacket.h"

class Tester : public CallBacker
{
public:

    void runEventLoopTest(CallBacker*)
    {
	prefix_ = "[event-loop] ";
	bool res = runTest( true );
	ApplicationData::exit( res ? 0 : 1 );
    }

    bool runTest( bool sendkill )
    {
	Network::RequestConnection conn( hostname_, (unsigned short)port_ );
	mAttachCB( conn.packetArrived, Tester::packetArrivedCB );

	Network::RequestPacket packet;
	BufferString sentmessage = "Hello World";
	packet.setIsNewRequest();
	packet.setStringPayload( sentmessage );

	mRunStandardTestWithError( conn.sendPacket( packet ),
				  BufferString( prefix_, "Sending packet 1"),
				  conn.errMsg().getFullString() );

	Network::RequestPacket packet2;
	packet2.setRequestID( packet.requestID() );
	packet2.setSubID( 1 );
	BufferString sentmessage2 = "Peace on Earth!";
	packet2.setStringPayload( sentmessage2 );

	mRunStandardTestWithError( conn.sendPacket( packet2 ),
				  BufferString( prefix_, "Sending packet 2"),
				  conn.errMsg().getFullString() );


	PtrMan<Network::RequestPacket> receivedpacket = 0;

	mRunStandardTestWithError(
	    receivedpacket=conn.pickupPacket( packet.requestID(), 2000 ),
	    BufferString( prefix_, "Receiving packet 1"),
	    conn.errMsg().getFullString() );

	BufferString receivedmessage1;
	receivedpacket->getStringPayload( receivedmessage1 );
	mRunStandardTest( receivedmessage1==sentmessage &&
			 receivedpacket->requestID()==packet.requestID() &&
			 receivedpacket->subID()==packet.subID(),
			 BufferString( prefix_, "Received content 1"));

	mRunStandardTestWithError(
		  receivedpacket=conn.pickupPacket( packet.requestID(), 2000 ),
		  BufferString( prefix_, "Receiving packet 2"),
		  conn.errMsg().getFullString() );

	BufferString receivedmessage2;
	receivedpacket->getStringPayload( receivedmessage2 );
	mRunStandardTest( receivedmessage2==sentmessage2 &&
			 receivedpacket->requestID()==packet2.requestID() &&
			 receivedpacket->subID()==packet2.subID(),
			 BufferString( prefix_, "Received content 2"));

/*
	{
	    Network::RequestPacket newpacket;
	    newpacket.setIsNewRequest();
	    newpacket.setStringPayload( "New" );

	    Threads::MutexLocker locker( condvar_ );
	    unexpectedarrived_ = false;

	    mRunStandardTestWithError( conn.sendPacket( newpacket ),
				BufferString( prefix_, "Sending newpacket"),
				conn.errMsg().getFullString() );

	    mRunStandardTest( condvar_.wait(10000) && unexpectedarrived_,
			     "External arrived" )
	}

*/


	if ( sendkill )
	{
	    Network::RequestPacket killpacket;
	    killpacket.setStringPayload("Kill");
	    killpacket.setIsNewRequest();

	    mRunStandardTestWithError( conn.sendPacket( killpacket ),
			  BufferString( prefix_, "Sending kill packet"),
			   conn.errMsg().getFullString() );
	}

	return true;
    }


    void packetArrivedCB(CallBacker* cb)
    {
	mCBCapsuleUnpackWithCaller( od_int32, reqid, cber, cb );
	mDynamicCastGet(Network::RequestConnection*, conn, cber );
	PtrMan<Network::RequestPacket> packet = conn->getNextExternalPacket();
	if ( packet )
	{
	    Threads::MutexLocker locker( condvar_ );
	    unexpectedarrived_ = true;
	    condvar_.signal( true );
	    reqid++; //Avoid unused warning.
	}
    }

    Threads::ConditionVar	condvar_;
    bool			unexpectedarrived_;

    BufferString		prefix_;
    BufferString		hostname_;
    int				port_;

};


int main(int argc, char** argv)
{
    mInitTestProg();
    ApplicationData app;

    Tester runner;
    runner.port_ = 1025;
    clparser.getVal( "port", runner.port_, true );
    runner.hostname_ = "localhost";
    runner.prefix_ = "[no event-loop] ";

    BufferString echoapp = "test_netreqechoserver";
    clparser.getVal( "serverapp", echoapp );

    BufferString args( "--port ", runner.port_ );

    if ( !clparser.hasKey("noechoapp") && !ExecODProgram( echoapp, args.buf() ))
    {
	od_ostream::logStream() << "Cannot start " << echoapp << "\n";
	ExitProgram( 1 );
    }

    Threads::sleep( 1 );

    if ( !runner.runTest(false) )
	ExitProgram( 1 );

    app.addToEventLoop( mCB( &runner,Tester, runEventLoopTest) );
    const int retval = app.exec();

    ExitProgram( retval );
}
