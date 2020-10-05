/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2013
-*/


#include "applicationdata.h"
#include "netserver.h"
#include "netsocket.h"
#include "genc.h"
#include "od_ostream.h"
#include "timer.h"
#include "testprog.h"

#include <time.h>

namespace Network
{

class EchoServer : public CallBacker
{
public:
    EchoServer( PortNr_Type startport, unsigned short timeout )
	: server_(false)
	, close_( false )
	, timeout_( timeout )
    {
	mAttachCB( server_.readyRead, EchoServer::dataArrivedCB );

	PortNr_Type port = startport;
	const PortNr_Type maxport = 10000;
	while ( !close_ && !server_.listen( Any, port++ ) &&
		port<maxport )
	{}

	mAttachCB( timer_.tick, EchoServer::timerTick );

	lastactivity_ = time( 0 );
	timer_.start( 1000, false );
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
		logStream() << "Read error" << od_endl;
		break;
	    }

	    logStream() << "\nEchoing " << readsize << " bytes" << od_endl;

	    const char* writeptr = data;
	    const FixedString writestr( writeptr+sizeof(int) );
	    if ( writestr.startsWith(Network::Server::sKeyKillword()) )
	    {
		CallBack::addToMainThread( mCB(this,EchoServer,closeServerCB) );
		return;
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
    }

    Network::Server		server_;
    Timer			timer_;
    time_t			lastactivity_;
    time_t			timeout_;
    bool			close_;
};

} //Namespace


int mTestMainFnName(int argc, char** argv)
{
    mInitTestProg();

    //Make standard test-runs just work fine.
    if ( !clParser().hasKey(Network::Server::sKeyPort()) )
	ExitProgram( 0 );

    ApplicationData app;

    clParser().setKeyHasValue( Network::Server::sKeyPort() );
    clParser().setKeyHasValue( Network::Server::sKeyTimeout() );

    int startport = 1025;
    clParser().getKeyedInfo( Network::Server::sKeyPort(), startport );
    int timeout = 120;
    clParser().getKeyedInfo( Network::Server::sKeyTimeout(), timeout );

    Network::EchoServer server( mCast(PortNr_Type,startport),
				mCast(unsigned short,timeout) );

    logStream() << "Listening to port " << server.server_.port()
		  << " with a " << server.timeout_ << " second timeout\n";

    return app.exec();
}
