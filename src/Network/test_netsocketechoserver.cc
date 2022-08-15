/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2013
-*/


#include "applicationdata.h"
#include "netserver.h"
#include "netsocket.h"
#include "timer.h"
#include "testprog.h"

#ifdef __win__
# include "time.h"
#endif

namespace Network
{

class EchoServer : public CallBacker
{
public:
    EchoServer( const Network::Authority& auth, unsigned short timeout )
	: server_(auth.isLocal())
	, timeout_( timeout )
    {
	uiRetVal uirv;
	const bool islistening = auth.isLocal()
		? server_.listen( auth.getServerName(), uirv )
		: server_.listen( auth.serverAddress(), auth.getPort() );
	if ( islistening )
	    mAttachCB( server_.readyRead, EchoServer::dataArrivedCB );

	mAttachCB(timer_.tick, EchoServer::timerTick);
	lastactivity_ = time(0);
	timer_.start(1000, false);
    }

    ~EchoServer()
    {
	detachAllNotifiers();
	CallBack::removeFromThreadCalls( this );
    }

    void dataArrivedCB( CallBacker* cb )
    {
	lastactivity_ = time( 0 );

	mCBCapsuleUnpack( int, socketid, cb );
	Network::Socket* socket = server_.getSocket( socketid );
#define mChunkSize 1000000
	char data[mChunkSize];
	while ( true )
	{
	    const od_int64 readsize = mMIN(mChunkSize,socket->bytesAvailable());
	    if ( !readsize )
		break;

	    if ( socket->readArray(data,readsize) != Network::Socket::ReadOK )
	    {
		errStream() << "Read error" << od_endl;
		break;
	    }

	    //logStream() << "\nEchoing " << readsize << " bytes" << od_endl;

	    const char* writeptr = data;
	    if ( readsize > 0 && readsize < 10 )
	    {
		const char* strptr = writeptr;
		/*With threads: the string size and string data come together,
		  the string being preceded by an integer */
		if ( readsize > sizeof(int) )
		    strptr += sizeof(int);
		const StringView writestr( strptr );
		if ( writestr.startsWith(Network::Server::sKeyKillword()) )
		{
		    socket->disconnectFromHost();
		    CallBack::addToMainThread(
			    mCB(this,EchoServer,closeServerCB) );
		    return;
		}

	    }

	    const od_int64 nrtowrite = readsize;
	    socket->writeArray( writeptr, nrtowrite );
	}
    }

    void closeServerCB( CallBacker* )
    {
	ApplicationData::exit( 0 );
    }

    void timerTick( CallBacker* )
    {
	const time_t curtime = time( 0 );
	if ( curtime-lastactivity_>timeout_ )
	{
	    logStream() << "Timeout" << od_endl;
	    CallBack::addToMainThread( mCB(this,EchoServer,closeServerCB) );
	}

	if ( !server_.isListening() )
	{
	    errStream() << "Server error: " << server_.errorMsg() << od_endl;
	    CallBack::addToMainThread( mCB(this,EchoServer,closeServerCB) );
	}

    }

    Network::Server		server_;
    Timer			timer_;
    time_t			lastactivity_;
    time_t			timeout_;
    bool			close_ = false;
};

} // namespace Network


int mTestMainFnName(int argc, char** argv)
{
    mInitTestProg();

    ApplicationData app;

    Network::Authority auth;
    auth.setFrom( clParser(), "test_netsocket",
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

    Network::EchoServer tester( auth, mCast(unsigned short,timeout) );

    logStream() << "Listening to " << auth.toString()
		<< " with a " << tester.timeout_ << " second timeout\n";

    return app.exec();
}
