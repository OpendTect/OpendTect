/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2013
-*/


#include "applicationdata.h"
#include "oscommand.h"
#include "ptrman.h"
#include "testprog.h"
#include "string.h"
#include "varlenarray.h"

#include "netreqconnection.h"
#include "netreqpacket.h"
#include "netserver.h"
#include "netsocket.h"


#define mLargePayload 50000000

static BufferString packetString( const char* prefix,
				  const char* start,
				  const Network::RequestPacket* packet )
{
    BufferString ret = prefix;
    ret.add( start );
    ret.add( " Request " ).add( packet->requestID() )
       .add( " SubID " ).add( packet->subID() );

    return ret;
}


class Tester : public CallBacker
{
public:
    ~Tester()
    {
	detachAllNotifiers();
	delete authority_;
	CallBack::removeFromThreadCalls( this );
    }

    void runEventLoopTest(CallBacker*)
    {
	prefix_ = "[multithreaded] ";
	bool res = runTest( true, true );
	ApplicationData::exit( res ? 0 : 1 );
    }

    bool runTest( bool sendkill, bool multithreaded )
    {
	Network::RequestConnection conn( *authority_,true, true,
							    multithreaded );
	mRunStandardTestWithError( conn.isOK(),
	      BufferString( prefix_, "Connection is OK"),
	      toString(conn.errMsg()) );
	conn_ = &conn;
	mAttachCB( conn.packetArrived, Tester::packetArrivedCB );

	RefMan<Network::RequestPacket> packet = new Network::RequestPacket;
	BufferString sentmessage = "Hello World";
	packet->setIsNewRequest();
	packet->setStringPayload( sentmessage );

	mRunStandardTestWithError( conn.sendPacket( *packet ),
	      packetString( prefix_, "Sending", packet ),
	      toString(conn.errMsg()) );

	RefMan<Network::RequestPacket> largepacket = new Network::RequestPacket;
	largepacket->setIsNewRequest();
	mAllocLargeVarLenArr( char, payload, mLargePayload+1 );
	memset( payload, ' ', mLargePayload );
	payload[mLargePayload] = 0;

	largepacket->setStringPayload( payload );

	mRunStandardTestWithError( conn.sendPacket( *largepacket ),
	      packetString( prefix_, "Sending large packet", largepacket ),
	      toString(conn.errMsg()) );

	RefMan<Network::RequestPacket> packet2 = new Network::RequestPacket;
	packet2->setRequestID( packet->requestID() );
	packet2->setSubID( 1 );
	BufferString sentmessage2 = "Peace on Earth!";
	packet2->setStringPayload( sentmessage2 );

	mRunStandardTestWithError( conn.sendPacket( *packet2 ),
	  packetString( prefix_, "Sending", packet2 ),
	  toString(conn.errMsg()) );

	RefMan<Network::RequestPacket> receivedpacket = 0;

	mRunStandardTestWithError(
	    receivedpacket=conn.pickupPacket( packet->requestID(), 20000 ),
	    packetString( prefix_, "Receiving", packet ),
	    toString(conn.errMsg()) );

	BufferString receivedmessage1;
	receivedpacket->getStringPayload( receivedmessage1 );
	mRunStandardTest( receivedmessage1==sentmessage &&
			 receivedpacket->requestID()==packet->requestID() &&
			 receivedpacket->subID()==packet->subID(),
			 packetString( prefix_, "Received content", packet ) );

	mRunStandardTestWithError(
		 receivedpacket=conn.pickupPacket( packet->requestID(), 20000 ),
		 packetString( prefix_, "Receiving", packet2 ),
		 toString(conn.errMsg()) );

	BufferString receivedmessage2;
	receivedpacket->getStringPayload( receivedmessage2 );
	mRunStandardTest( receivedmessage2==sentmessage2 &&
			 receivedpacket->requestID()==packet2->requestID() &&
			 receivedpacket->subID()==packet2->subID(),
			 packetString( prefix_, "Received content", packet2 ) );

	mRunStandardTestWithError(
	    receivedpacket=conn.pickupPacket( largepacket->requestID(), 20000 ),
	    packetString( prefix_, "Receiving large", largepacket ),
	    toString(conn.errMsg()) );

	BufferString receivedlongmessage;
	receivedpacket->getStringPayload( receivedlongmessage );
	mRunStandardTest(
	     receivedlongmessage==payload &&
	     receivedpacket->requestID()==largepacket->requestID() &&
	     receivedpacket->subID()==largepacket->subID(),
	     packetString( prefix_, "Large packet content", largepacket ));

	if ( !sendPacketInOtherThread() )
	    return false;

	{
	    //Send a packet that requests a disconnect. Server will
	    //disconnect, and we should not be able to read the packet.
	    //
	    //Further, the errorcode should be set correctly.

	    Network::RequestConnection conn2( *authority_, true, true,
							    multithreaded );
	    mRunStandardTestWithError( conn2.isOK(),
	      BufferString( prefix_, "Connection 2 is OK"),
	      toString(conn.errMsg()) );

	    RefMan<Network::RequestPacket> disconnectpacket =
						new Network::RequestPacket;
	    disconnectpacket->setIsNewRequest();
	    disconnectpacket->setStringPayload( "Disconnect" );

	    mRunStandardTestWithError( conn2.sendPacket( *disconnectpacket ),
	      packetString( prefix_, "Sending disconnect", disconnectpacket ),
	      toString(conn2.errMsg()) );

	    int errorcode = 0;
	    mRunStandardTest(
	      !(receivedpacket=conn2.pickupPacket(disconnectpacket->requestID(),
		  20000, &errorcode )),
	      packetString( prefix_, "Receiving disconnect should fail",
		  disconnectpacket ) );

	    mRunStandardTest(errorcode==conn2.cDisconnected(),
	      packetString( prefix_, "Errorcode == disconnection",
			    disconnectpacket ) );
	}

	if ( sendkill )
	{
	    RefMan<Network::RequestPacket> killpacket =
						new Network::RequestPacket;
	    killpacket->setStringPayload( Network::Server::sKeyKillword() );
	    killpacket->setIsNewRequest();

	    mRunStandardTestWithError( conn.sendPacket( *killpacket ),
			  BufferString( prefix_, "Sending kill packet"),
			   toString(conn.errMsg()) );
	}

	return true;
    }


    void packetArrivedCB(CallBacker* cb)
    {
	mCBCapsuleUnpackWithCaller( od_int32, reqid, cber, cb );
	mDynamicCastGet(Network::RequestConnection*, conn, cber );
	RefMan<Network::RequestPacket> packet = conn->getNextExternalPacket();
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
	Threads::Thread thread( mCB(this,Tester,threadSend),
				"threadSend thread" );
	thread.waitForFinish();
	return sendres_;
    }

    void threadSend(CallBacker*)
    {
	sendres_ = sendMessageInThread();
    }

    bool sendMessageInThread()
    {
	RefMan<Network::RequestPacket> packet = new Network::RequestPacket;
	BufferString sentmessage = "Hello World";
	packet->setIsNewRequest();
	packet->setStringPayload( sentmessage );

	mRunStandardTestWithError(
	      conn_->sendPacket( *packet )==conn_->isMultiThreaded(),
	      packetString( prefix_, "Sending from other thread", packet ),
	      toString(conn_->errMsg()) );

	RefMan<Network::RequestPacket> receivedpacket =
	    conn_->pickupPacket( packet->requestID(), 20000 );
	mRunStandardTestWithError(
	    ((bool) receivedpacket)==conn_->isMultiThreaded(),
	    packetString( prefix_, "Receiving from other thread", packet ),
	    toString(conn_->errMsg()) );

	if ( receivedpacket )
	{
	    BufferString receivedmessage1;
	    receivedpacket->getStringPayload( receivedmessage1 );
	    mRunStandardTest( receivedmessage1==sentmessage &&
		 receivedpacket->requestID()==packet->requestID() &&
		 receivedpacket->subID()==packet->subID(),
		 packetString( prefix_, "Received content from other thread",
			       packet ) );
	}


	return true;
    }


    Threads::ConditionVar	condvar_;
    bool			unexpectedarrived_;

    Network::RequestConnection* conn_;
    bool			sendres_;

    BufferString		prefix_;
    Network::Authority*		authority_ = nullptr;
};


int mTestMainFnName(int argc, char** argv)
{
    mInitTestProg();
    ApplicationData app;
    const char* portkey = Network::Server::sKeyPort();
    clParser().setKeyHasValue( portkey );
    clParser().setKeyHasValue( "serverapp" );

    PtrMan<Tester> runner = new Tester;
    int port = 1025;
    clParser().getKeyedInfo( portkey, port, true );
    delete runner->authority_;
    runner->authority_ = new Network::Authority(
	Network::Socket::sKeyLocalHost(), mCast(PortNr_Type,port) );
    runner->prefix_ = "[singlethreaded] ";

    BufferString echoapp = "test_netreqechoserver";
    clParser().getKeyedInfo( "serverapp", echoapp );
    OS::MachineCommand echocmd( echoapp );
    echocmd.addKeyedArg( portkey, runner->authority_->getPort() );
    echocmd.addFlag( "quiet" );

    OS::CommandExecPars execpars( OS::RunInBG );
    OS::CommandLauncher cl( echocmd );

    if ( !clParser().hasKey("noechoapp")
      && !cl.execute(execpars))
    {
	od_ostream::logStream() << "Cannot start " << echoapp << "\n";
	return 1;
    }

    Threads::sleep( 1 );

    if ( !runner->runTest(false,false) )
	return 1;

    CallBack::addToMainThread( mCB(runner,Tester,runEventLoopTest) );
    const int retval = app.exec();

    runner = 0;

    return retval;
}
