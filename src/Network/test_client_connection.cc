/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "applicationdata.h"
#include "netreqpacket.h"
#include "netreqconnection.h"
#include "netserver.h"
#include "netsocket.h"
#include "testprog.h"
#include "timer.h"

#ifdef __win__
# include "time.h"
#endif

static const char* addrkey = "address";
static const char* portkey = Network::Server::sKeyPort();
static const char* msgkey = "msg";
static const char* defmsg = "Hello, World";

static void printBatchUsage()
{
    od_ostream& strm = logStream();
    strm << "Usage: " << "test_client_connection [-h] "
			 "[--address ADDRESS] [--port PORT]\n\n";
    strm << "Client application for connection to TCP server\n\n";
    strm << "optional arguments:\n";
    strm << "  -h, --help\t\tshow this help message and exit\n\n";
    strm << "  --" << addrkey << " ADDRESS\tAddress to connect to "
				 "(default: localhost)\n";
    strm << "  --" << portkey << " PORT\t\t"
				 "Port to connect to (default: 37504)\n";
    strm << "  --" << msgkey << " MESSAGE\t\tMessage to send";
    strm << od_endl;
}


namespace Network
{

class Tester : public CallBacker
{
public:
    Tester( const Authority& auth, const char* msg )
	: authority_(auth)
	, msg_(msg)
    {}

    ~Tester()
    {
	detachAllNotifiers();
	CallBack::removeFromThreadCalls( this );
    }

    void runEventLoopTest( CallBacker* )
    {
	const bool res = runTest();
	ApplicationData::exit( res ? 0 : 1 );
    }

    const char* message() const		{ return outmsg_.buf(); }
    uiString errMsg() const		{ return errmsg_; }

private:

    bool runTest()
    {
	Network::RequestConnection conn( authority_ );
	if ( !conn.isOK() )
	    { errmsg_ = conn.errMsg(); return false; }

	logStream() << "[OK] Connection established" << od_newline;

	RefMan<Network::RequestPacket> packet = new Network::RequestPacket;
	packet->setIsNewRequest();
	packet->setStringPayload( msg_.buf() );

	if ( !conn.sendPacket(*packet) )
	    { errmsg_ = conn.errMsg(); return false; }

	RefMan<Network::RequestPacket> receivedpacket =
			conn.pickupPacket( packet->requestID(), 20000 );
	if ( !receivedpacket )
	    { errmsg_ = conn.errMsg(); return false; }

	receivedpacket->getStringPayload( outmsg_ );
	if ( outmsg_.isEmpty() )
	    { errmsg_ = toUiString("Received empty message"); return false; }

	return true;
    }

    Authority				authority_;
    uiString				errmsg_;
    BufferString			msg_;
    BufferString			outmsg_;

};

} // namespace Network


int mTestMainFnName(int argc, char** argv)
{
    mInitTestProg();

    ApplicationData app;

    CommandLineParser& parser = clParser();
    parser.setKeyHasValue( addrkey );
    parser.setKeyHasValue( portkey );
    parser.setKeyHasValue( msgkey );

    if ( parser.hasKey("help") || parser.hasKey("h") )
    {
	printBatchUsage();
	return 1;
    }

    PortNr_Type port = 37504;
    parser.getVal( portkey, port );

    BufferString addr( Network::Socket::sKeyLocalHost() );
    parser.getVal( addrkey, addr );

    BufferString msg;
    if ( !parser.getVal(msgkey,msg) )
	msg.set( defmsg );

    logStream() << "Will try to connect to host '" << addr.buf()
		<< "' using port: " << port << od_endl;

    const Network::Authority auth( addr.buf(), port );
    Network::Tester runner( auth, msg.buf() );
    CallBack::addToMainThread( mCB(&runner,Network::Tester,runEventLoopTest) );
    const int retval = app.exec();
    if ( retval == 0 )
	logStream() << "[OK] Received back '"
		    << runner.message() << "'" << od_endl;
    else
	errStream() << "[FAIL] " << toString( runner.errMsg() ) << od_endl;

    return retval;
}
