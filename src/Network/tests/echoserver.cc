/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2013
-*/

static const char* rcsID mUsedVar = "$Id$";

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
	: close_( false )
	, timeout_( timeout )
	, server_(false)
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
	CallBack::removeFromMainThread( this );
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
		if ( !quiet )
		    od_cout() << "Read error" << od_endl;
		break;
	    }

	    //od_cout() << "\nEchoing " << readsize << " bytes" << od_endl;

	    const char* writeptr = data;
	    if ( readsize > 0 && readsize < 10 )
	    {
		const char* strptr = writeptr;
		/*With threads: the string size and string data come together,
		  the string being preceded by an integer */
		if ( readsize > sizeof(int) )
		    strptr += sizeof(int);
		const FixedString writestr( strptr );
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
	    if ( !quiet )
		od_cout() << "Timeout" << od_endl;

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


int main(int argc, char** argv)
{
    mInitTestProg();

    //Make standard test-runs just work fine.
    if ( !clparser.hasKey(Network::Server::sKeyPort()) )
	ExitProgram( 0 );

    ApplicationData app;

    int startport = 1025;
    clparser.getVal( Network::Server::sKeyPort(), startport );

    int timeout = 120;
    clparser.getVal( Network::Server::sKeyTimeout(), timeout );

    Network::EchoServer server( mCast(PortNr_Type,startport),
				mCast(unsigned short,timeout) );

    if ( !quiet )
    {
	od_cout() << "Listening to port " << server.server_.port()
		  << " with a " << server.timeout_ << " second timeout\n";
    }

    ExitProgram( app.exec() );
}
