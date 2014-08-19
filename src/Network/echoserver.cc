/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2013
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "tcpconnection.h"

#include "commandlineparser.h"
#include "tcpserver.h"
#include "tcpsocket.h"
#include "applicationdata.h"
#include "genc.h"
#include "od_ostream.h"
#include "time.h"
#include "timer.h"

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
	TcpSocket* socket = server_.getSocket( socketid );

	char data[1024];
	while ( true )
	{
	    const int nrread = socket->readdata( data, 1024 );
	    if ( !nrread )
		break;

	    const char* writeptr = data;
	    int nrtowrite = nrread;
	    while ( nrtowrite )
	    {
		const int nrwritten = socket->writedata( writeptr, nrtowrite );
		nrtowrite -= nrwritten;
		writeptr += nrwritten;
	    }
	}
    }

    TcpServer			server_;
    bool			close_;
    time_t			lasttime_;
    int				timeout_;
    Timer			timer_;
};


int main(int argc, char** argv)
{
    SetProgramArgs( argc, argv );
    ApplicationData app;

    CommandLineParser clparser;

    int startport = 1025;
    clparser.getVal( "port", startport );

    EchoServer server( startport );

    clparser.getVal( "timeout", server.timeout_ );

    od_cout() << "Listening to port " << server.server_.port()
	      << " with a " <<server.timeout_ << " second timeout\n";


    ExitProgram( app.exec() );
}
