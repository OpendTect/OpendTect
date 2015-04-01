/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2013
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "applicationdata.h"
#include "oscommand.h"
#include "ptrman.h"
#include "testprog.h"
#include "string.h"
#include "varlenarray.h"

#include "netreqconnection.h"
#include "netreqpacket.h"


#define mLargePayload 50000000

static BufferString packetString( const char* prefix,
				  const char* start,
				  const Network::RequestPacket& packet )
{
    BufferString ret = prefix;
    ret.add( start );
    ret.add( " Request " ).add( packet.requestID() )
       .add( " SubID " ).add( packet.subID() );

    return ret;
}


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
	conn_ = &conn;
	mAttachCB( conn.packetArrived, Tester::packetArrivedCB );

	Network::RequestPacket packet;
	BufferString sentmessage = "Hello World";
	packet.setIsNewRequest();
	packet.setStringPayload( sentmessage );

	mRunStandardTestWithError( conn.sendPacket( packet ),
	      packetString( prefix_, "Sending", packet ),
	      conn.errMsg().getFullString() );

	Network::RequestPacket largepacket;
	largepacket.setIsNewRequest();
	mAllocLargeVarLenArr( char, payload, mLargePayload+1 );
	memset( payload, ' ', mLargePayload );
	payload[mLargePayload] = 0;

	largepacket.setStringPayload( payload );

	mRunStandardTestWithError( conn.sendPacket( largepacket ),
	      packetString( prefix_, "Sending large packet", largepacket ),
	      conn.errMsg().getFullString() );

	Network::RequestPacket packet2;
	packet2.setRequestID( packet.requestID() );
	packet2.setSubID( 1 );
	BufferString sentmessage2 = "Peace on Earth!";
	packet2.setStringPayload( sentmessage2 );

	mRunStandardTestWithError( conn.sendPacket( packet2 ),
	  packetString( prefix_, "Sending", packet2 ),
	  conn.errMsg().getFullString() );

	PtrMan<Network::RequestPacket> receivedpacket = 0;

	mRunStandardTestWithError(
	    receivedpacket=conn.pickupPacket( packet.requestID(), 2000 ),
	    packetString( prefix_, "Receiving", packet ),
	    conn.errMsg().getFullString() );

	BufferString receivedmessage1;
	receivedpacket->getStringPayload( receivedmessage1 );
	mRunStandardTest( receivedmessage1==sentmessage &&
			 receivedpacket->requestID()==packet.requestID() &&
			 receivedpacket->subID()==packet.subID(),
			 packetString( prefix_, "Received content", packet ) );

	mRunStandardTestWithError(
		  receivedpacket=conn.pickupPacket( packet.requestID(), 2000 ),
		  packetString( prefix_, "Receiving", packet2 ),
		  conn.errMsg().getFullString() );

	BufferString receivedmessage2;
	receivedpacket->getStringPayload( receivedmessage2 );
	mRunStandardTest( receivedmessage2==sentmessage2 &&
			 receivedpacket->requestID()==packet2.requestID() &&
			 receivedpacket->subID()==packet2.subID(),
			 packetString( prefix_, "Received content", packet2 ) );

	mRunStandardTestWithError(
	    receivedpacket=conn.pickupPacket( largepacket.requestID(), 2000 ),
	    packetString( prefix_, "Receiving large", largepacket ),
	    conn.errMsg().getFullString() );

	BufferString receivedlongmessage;
	receivedpacket->getStringPayload( receivedlongmessage );
	mRunStandardTest( 
	     receivedlongmessage==payload &&
	     receivedpacket->requestID()==largepacket.requestID() &&
	     receivedpacket->subID()==largepacket.subID(),
	     packetString( prefix_, "Large packet content", largepacket ));

	if ( !sendPacketInOtherThread() )
	    return false;

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

    bool sendPacketInOtherThread()
    {
	sendres_ = false;
	Threads::Thread thread( mCB(this,Tester,threadSend) );
	thread.waitForFinish();
	return sendres_;
    }

    void threadSend(CallBacker*)
    {
	sendres_ = sendMessageInThread();
    }

    bool sendMessageInThread()
    {
	Network::RequestPacket packet;
	BufferString sentmessage = "Hello World";
	packet.setIsNewRequest();
	packet.setStringPayload( sentmessage );

	mRunStandardTestWithError( conn_->sendPacket( packet ),
	      packetString( prefix_, "Sending from other thread", packet ),
	      conn_->errMsg().getFullString() );

	PtrMan<Network::RequestPacket> receivedpacket = 0;
	mRunStandardTestWithError(
	    receivedpacket=conn_->pickupPacket( packet.requestID(), 2000 ),
	    packetString( prefix_, "Receiving from other thread", packet ),
	    conn_->errMsg().getFullString() );

	BufferString receivedmessage1;
	receivedpacket->getStringPayload( receivedmessage1 );
	mRunStandardTest( receivedmessage1==sentmessage &&
	     receivedpacket->requestID()==packet.requestID() &&
	     receivedpacket->subID()==packet.subID(),
	     packetString( prefix_, "Received content from other thread",
			   packet ) );


	return true;
    }


    Threads::ConditionVar	condvar_;
    bool			unexpectedarrived_;

    Network::RequestConnection* conn_;
    bool			sendres_;

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
    args.add( " --quiet " );

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
