/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2013
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "commandlineparser.h"
#include "netserver.h"
#include "netsocket.h"
#include "applicationdata.h"
#include "genc.h"
#include "od_ostream.h"
#include "time.h"
#include "timer.h"
#include "testprog.h"

class EchoServer : public CallBacker
{
public:
    EchoServer( int startport )
	: close_( false )
	, timeout_( 60 )
    {
	mAttachCB( server_.readyRead, EchoServer::dataArrived );

	int port = startport;
	const int maxport = 10000;
	while ( !close_ && !server_.listen( 0, port++ ) && port<maxport )
	{}

	mAttachCB( timer_.tick, EchoServer::timerClick );

	timer_.start( timeout_*1000, true );
	lasttime_ = time( 0 );
    }

    ~EchoServer()
    {
	detachAllNotifiers();
    }

    void timerClick(CallBacker*)
    {
	const time_t curtime = time( 0 );
	if ( curtime-lasttime_>timeout_ )
	{
	    if ( !quiet )
		od_cout() << "Timeout" << od_endl;
	    server_.close();
	    ApplicationData::exit( 0 );
	}
	else
	{
	    timer_.start( timeout_*1000, true );
	}
    }

    void dataArrived( CallBacker* cb )
    {
	lasttime_ = time( 0 );

	mCBCapsuleUnpack( int, socketid, cb );
	Network::Socket* socket = server_.getSocket( socketid );
#define mChunkSize 1000000
	char data[mChunkSize];
	while ( true )
	{
	    const od_int64 readsize = mMIN(mChunkSize,socket->bytesAvailable());
	    if ( !readsize )
		break;

	    if ( socket->readArray( data, readsize )!=Network::Socket::ReadOK )
	    {
		if ( !quiet )
		    od_cout() << "Read error" << od_endl;
		break;
	    }

	    if ( !quiet )
		od_cout() << "Echoing " << readsize << " bytes" << od_endl;

	    const char* writeptr = data;
	    const od_int64 nrtowrite = readsize;;
	    socket->writeArray( writeptr, nrtowrite );
	}
    }

    Network::Server		server_;
    bool			close_;
    time_t			lasttime_;
    int				timeout_;
    Timer			timer_;
};


int main(int argc, char** argv)
{
    mInitTestProg();
    if ( !clparser.hasKey("port") )
	ExitProgram( 0 );


    SetProgramArgs( argc, argv );
    ApplicationData app;

    int startport = 1025;
    clparser.getVal( "port", startport );

    EchoServer server( startport );

    clparser.getVal( "timeout", server.timeout_ );

    if ( !quiet )
	od_cout() << "Listening to port " << server.server_.port()
	      << " with a " <<server.timeout_ << " second timeout\n";


    ExitProgram( app.exec() );
}

