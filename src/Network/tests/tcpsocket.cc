/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2013
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "tcpsocket.h"

#include "applicationdata.h"
#include "oscommand.h"
#include "odmemory.h"
#include "statrand.h"
#include "varlenarray.h"
#include "limits.h"
#include "odsysmem.h"
#include "tcpserver.h"
#include "testprog.h"
#include "thread.h"



Stats::RandGen gen;

double randVal() { return gen.get(); }

od_int64 nrdoubles;
ArrPtrMan<double> doublewritearr, doublereadarr;

#define mRunTcpTest( test, msg ) \
    mRunStandardTestWithError( test, BufferString( prefix_, msg ), \
			       connection.errMsg().getFullString()  )

class TestRunner : public CallBacker
{
public:
    bool	testTcpSocket();

    void	testCallBack(CallBacker*)
    {
	const bool testresult = testTcpSocket();
	if ( exitonfinish_ ) ApplicationData::exit( testresult ? 0 : 1 );
    }

    bool		noeventloop_;
    int			port_;
    const char*		prefix_;
    bool		exitonfinish_;
    BufferString	serverapp_;
    BufferString	serverarg_;
};


bool TestRunner::testTcpSocket()
{
    TcpSocket connection( !noeventloop_ );
    connection.setTimeout( 10000 );

    if ( !connection.connectToHost( "localhost", port_, true ) )
    {
	if ( !ExecODProgram( serverapp_, serverarg_ ) )
	{
	    od_ostream::logStream() << "Cannot start " << serverapp_;
	    return false;
	}

	Threads::sleep( 15 );
    }
    else
    {
	connection.abort();
    }

    mRunTcpTest(
	    !connection.connectToHost( "non_existing_host", 20000, true ),
	    "Connect to non-existing host");

    mRunTcpTest(
	    connection.connectToHost( "localhost", port_, true ),
	    "Connect to echo server");

    BufferString writebuf = "Hello world";
    const int writesize = writebuf.size()+1;
    mRunTcpTest(
	    connection.writeArray( writebuf.buf(), writesize, true ),
	    "writeArray & wait to echo server" );

    char readbuf[1024];

    mRunTcpTest( connection.readArray( readbuf, writesize ),
		  "readArray after write & wait" );

    mRunTcpTest( writebuf==readbuf,
		  "Returned data identical to sent data after write & wait");

    mRunTcpTest(
	    connection.writeArray( writebuf.buf(), writesize, false ),
	    "writeArray & leave to echo server" );

    mRunTcpTest( connection.readArray( readbuf, writesize ),
		 "readArray after write & leave" );

    mRunTcpTest( writebuf==readbuf,
		  "Returned data identical to sent data after write & leave");

    mRunTcpTest(
	    connection.writeArray( writebuf.buf(), writesize, true ) &&
	    !connection.readArray( readbuf, writesize+1 ),
	    "Reading more than available should timeout and fail" );

    BufferString readstring;

    mRunTcpTest(
	    connection.write( writebuf ) &&
	    connection.read( readstring ) &&
	    readstring == writebuf,
	    "Sending and reading a string" );

    //The echo server cannot handle more than 2GB for some reason. Probably
    //because the buffers will overflow as we will not start reading
    //before everything is written.

    mRunTcpTest(
	    connection.writeDoubleArray( doublewritearr, nrdoubles, false ),
	    "Write large array" );

    mRunTcpTest( connection.readDoubleArray( doublereadarr, nrdoubles ),
	    "Read large array" );

    bool readerror = false;
    for ( int idx=0; idx<nrdoubles; idx++ )
    {
	if ( doublewritearr[idx] != doublereadarr[idx] )
	{
	    readerror = true;
	    break;
	}
    }

    mRunTcpTest( !readerror, "Large array integrity" );

    return true;
}


// Should operate against a server that echos all input back to sender, which
// can be specified by --serverapp "application". If no serverapp is given,
// echoserver is started

int main(int argc, char** argv)
{
    mInitTestProg();
    ApplicationData app;

    BufferString serverapp = "echoserver";

    TestRunner runner;
    runner.serverapp_ = "echoserver";
    runner.serverarg_ = "--timeout 72000 --port 1025";
    runner.port_ = 1025;
    runner.prefix_ = "[ No event loop ]\t";
    runner.exitonfinish_ = false;
    runner.noeventloop_ = true;

    clparser.getVal( "serverapp", runner.serverapp_, true );
    clparser.getVal( "serverarg", runner.serverarg_, true );
    clparser.getVal( "port", runner.port_, true );

    od_int64 totalmem, freemem;
    OD::getSystemMemory( totalmem, freemem );

    nrdoubles = freemem/sizeof(double);
    nrdoubles /= 4;
    nrdoubles = mMIN(nrdoubles,200000000); //1.6GB
    nrdoubles = mMAX(nrdoubles,2000); //Gotta send something

    doublewritearr = new double[nrdoubles];
    doublereadarr = new double[nrdoubles];

    MemSetter<double> memsetter( doublewritearr, 0, nrdoubles );
    memsetter.setValueFunc( &randVal );
    memsetter.execute();

    if ( !runner.testTcpSocket() )
	ExitProgram( 1 );

    //Now with a running event loop

    runner.prefix_ = "[ With event loop ]\t";
    runner.exitonfinish_ = true;
    runner.noeventloop_ = false;
    app.addToEventLoop( mCB(&runner,TestRunner,testCallBack) );
    const int res = app.exec();

    ExitProgram( res );
}
