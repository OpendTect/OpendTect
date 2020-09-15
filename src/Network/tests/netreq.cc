/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2013
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "applicationdata.h"
#include "oscommand.h"
#include "ptrman.h"
#include "sighndl.h"
#include "string.h"
#include "testprog.h"
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

    Tester( const Network::Authority& auth )
	: authority_(auth)
    {}

    ~Tester()
    {
	detachAllNotifiers();
	CallBack::removeFromMainThread( this );
    }

    void runEventLoopTest(CallBacker*)
    {
	prefix_ = "[multithreaded] ";
	bool res = runTest( true, true );
	ApplicationData::exit( res ? 0 : 1 );
    }

    bool runTest( bool sendkill, bool multithreaded )
    {
	Network::RequestConnection conn( authority_, multithreaded );
	mRunStandardTestWithError( conn.isOK(),
	      BufferString( prefix_, "Connection is OK"),
	      conn.errMsg().getFullString() );
	conn_ = &conn;
	mAttachCB( conn.packetArrived, Tester::packetArrivedCB );

	PtrMan<Network::RequestPacket> packet = new Network::RequestPacket;
	BufferString sentmessage = "Hello World";
	packet->setIsNewRequest();
	packet->setStringPayload( sentmessage );

	mRunStandardTestWithError( conn.sendPacket( *packet ),
	      packetString( prefix_, "Sending", packet ),
	      conn.errMsg().getFullString() );

	PtrMan<Network::RequestPacket> largepacket = new Network::RequestPacket;
	largepacket->setIsNewRequest();
	mAllocLargeVarLenArr( char, payload, mLargePayload+1 );
	memset( payload, ' ', mLargePayload );
	payload[mLargePayload] = 0;

	largepacket->setStringPayload( payload );

	mRunStandardTestWithError( conn.sendPacket( *largepacket ),
	      packetString( prefix_, "Sending large packet", largepacket ),
	      conn.errMsg().getFullString() );

	PtrMan<Network::RequestPacket> packet2 = new Network::RequestPacket;
	packet2->setRequestID( packet->requestID() );
	packet2->setSubID( 1 );
	BufferString sentmessage2 = "Peace on Earth!";
	packet2->setStringPayload( sentmessage2 );

	mRunStandardTestWithError( conn.sendPacket( *packet2 ),
	  packetString( prefix_, "Sending", packet2 ),
	  conn.errMsg().getFullString() );

	PtrMan<Network::RequestPacket> receivedpacket = 0;

	mRunStandardTestWithError(
	    receivedpacket=conn.pickupPacket( packet->requestID(), 20000 ),
	    packetString( prefix_, "Receiving", packet ),
	    conn.errMsg().getFullString() );

	BufferString receivedmessage1;
	receivedpacket->getStringPayload( receivedmessage1 );
	mRunStandardTest( receivedmessage1==sentmessage &&
			 receivedpacket->requestID()==packet->requestID() &&
			 receivedpacket->subID()==packet->subID(),
			 packetString( prefix_, "Received content", packet ) );

	mRunStandardTestWithError(
		 receivedpacket=conn.pickupPacket( packet->requestID(), 20000 ),
		 packetString( prefix_, "Receiving", packet2 ),
		 conn.errMsg().getFullString() );

	BufferString receivedmessage2;
	receivedpacket->getStringPayload( receivedmessage2 );
	mRunStandardTest( receivedmessage2==sentmessage2 &&
			 receivedpacket->requestID()==packet2->requestID() &&
			 receivedpacket->subID()==packet2->subID(),
			 packetString( prefix_, "Received content", packet2 ) );

	mRunStandardTestWithError(
	    receivedpacket=conn.pickupPacket( largepacket->requestID(), 20000 ),
	    packetString( prefix_, "Receiving large", largepacket ),
	    conn.errMsg().getFullString() );

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

	    Network::RequestConnection conn2( authority_,
					      multithreaded );
	    mRunStandardTestWithError( conn2.isOK(),
	      BufferString( prefix_, "Connection 2 is OK"),
	      conn.errMsg().getFullString() );

	    PtrMan<Network::RequestPacket> disconnectpacket =
						new Network::RequestPacket;
	    disconnectpacket->setIsNewRequest();
	    disconnectpacket->setStringPayload( "Disconnect" );

	    mRunStandardTestWithError( conn2.sendPacket( *disconnectpacket ),
	      packetString( prefix_, "Sending disconnect", disconnectpacket ),
	      conn2.errMsg().getFullString() );

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
	    PtrMan<Network::RequestPacket> killpacket =
						new Network::RequestPacket;
	    killpacket->setStringPayload( Network::Server::sKeyKillword() );
	    killpacket->setIsNewRequest();

	    mRunStandardTestWithError( conn.sendPacket( *killpacket ),
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
	PtrMan<Network::RequestPacket> packet = new Network::RequestPacket;
	BufferString sentmessage = "Hello World";
	packet->setIsNewRequest();
	packet->setStringPayload( sentmessage );

	mRunStandardTestWithError(
	      conn_->sendPacket( *packet )==conn_->isMultiThreaded(),
	      packetString( prefix_, "Sending from other thread", packet ),
	      conn_->errMsg().getFullString() );

	PtrMan<Network::RequestPacket> receivedpacket =
	    conn_->pickupPacket( packet->requestID(), 20000 );
	mRunStandardTestWithError(
	    ((bool) receivedpacket)==conn_->isMultiThreaded(),
	    packetString( prefix_, "Receiving from other thread", packet ),
	    conn_->errMsg().getFullString() );

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
    bool			unexpectedarrived_ = false;

    Network::RequestConnection* conn_ = nullptr;
    bool			sendres_ = false;

    BufferString		prefix_;
    Network::Authority		authority_;
};


static void terminateServer( const PID_Type pid )
{
    Threads::sleep( 0.1 );
    if ( pid < 1 || !isProcessAlive(pid) )
        return;

    od_cout() << "Terminating zombie server with PID: " << pid << od_endl;
    SignalHandling::stopProcess( pid );
}


int main( int argc, char** argv )
{
    mInitTestProg();

    ApplicationData app;

    const Network::Authority auth = Network::Authority::getFrom( clparser,
		 "test_netreq",
		  Network::Socket::sKeyLocalHost(), PortNr_Type(1025) );
    if ( !auth.isUsable() )
    {
	od_ostream& strm = od_ostream::logStream();
	strm << "Incorrect authority '" << auth.toString() << "'";
	strm << "for starting the server" << od_endl;
	return 1;
    }

    PID_Type serverpid = -1;
    if ( !clparser.hasKey("noechoapp") )
    {
	BufferString echoapp = "test_netreqechoserver";
	clparser.setKeyHasValue( "serverapp" );
	clparser.getVal( "serverapp", echoapp );

	OS::MachineCommand mc( echoapp );
	auth.addTo( mc );
	//if ( clparser.hasKey(sKey::Quiet()) )
	    mc.addFlag( sKey::Quiet() );

	const OS::CommandExecPars execpars( OS::RunInBG );
	OS::CommandLauncher cl(mc);
	if ( !cl.execute(execpars) )
	{
	    od_ostream& strm = od_ostream::logStream();
	    strm << "Cannot start " << mc.toString( &execpars );
	    strm << ": " << toString(cl.errorMsg()) << od_endl;
	    return 1;
	}

	Threads::sleep( 1 );
	serverpid = cl.processID();
	mRunStandardTest( (isProcessAlive(serverpid)),
			BufferString("Server started with PID: ", serverpid) );
    }

    PtrMan<Tester> runner = new Tester( auth );
    runner->prefix_ = "[singlethreaded] ";
    if ( !runner->runTest(false,false) )
    {
	terminateServer( serverpid );
	ExitProgram( 1 );
    }

    //Now with a running event loop

    CallBack::addToMainThread( mCB(runner,Tester,runEventLoopTest) );
    const int retval = app.exec();

    runner = nullptr;

    if ( serverpid > 0 )
    {
	Threads::sleep(1);
	mRunStandardTest( (!isProcessAlive(serverpid)),
			  "Server has been stopped" );
	terminateServer( serverpid );
    }

    ExitProgram( retval );
}
