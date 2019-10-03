/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2013
-*/


#include "netsocket.h"

#include "applicationdata.h"
#include "limits.h"
#include "netreqconnection.h"
#include "netserver.h"
#include "odsysmem.h"
#include "odmemory.h"
#include "oscommand.h"
#include "statrand.h"
#include "testprog.h"
#include "thread.h"
#include "varlenarray.h"



Stats::RandGen gen;

double randVal() { return gen.get(); }

od_int64 nrdoubles;
ArrPtrMan<double> doublewritearr, doublereadarr;

#define mRunSockTest( test, msg ) \
    mRunStandardTestWithError( (test), BufferString( prefix_, msg ), \
			       toString(connection.errMsg()) )

class TestRunner : public CallBacker
{
public:
    ~TestRunner()
    {
	detachAllNotifiers();
	CallBack::removeFromThreadCalls( this );
    }

    bool	testNetSocket(bool closeserver=false);

    void	testCallBack(CallBacker*)
    {
	const bool testresult = testNetSocket( exitonfinish_ );
	if ( exitonfinish_ ) ApplicationData::exit( testresult ? 0 : 1 );
    }

    bool		noeventloop_;
    int			port_;
    int			timeout_;
    const char*		prefix_;
    bool		exitonfinish_;
    OS::MachineCommand	servercmd_;
};


bool TestRunner::testNetSocket( bool closeserver )
{
    Network::Socket connection( !noeventloop_ );
    connection.setTimeout( 600 );

    if ( !connection.connectToHost(Network::Socket::sKeyLocalHost(),
				   port_,true) )
    {
	OS::CommandLauncher cl( servercmd_ );
	if ( !cl.execute(OS::RunInBG) )
	{
	    od_ostream::logStream() << "Cannot start " << servercmd_.program()
				<< ": " << toString(cl.errorMsg()) << od_endl;
	    return false;
	}

	Threads::sleep( 15 );
    }
    else
    {
	connection.abort();
    }

    mRunSockTest(
	    !connection.connectToHost( "non_existing_host", 20000, true ),
	    "Connect to non-existing host");

    mRunSockTest(
	   connection.connectToHost(Network::Socket::sKeyLocalHost(),
				    port_,true), "Connect to echo server");

    BufferString writebuf = "Hello world";
    const int writesize = writebuf.size()+1;
    mRunSockTest(
	    connection.writeArray( writebuf.buf(), writesize, true ),
	    "writeArray & wait to echo server" );

    char readbuf[1024];

    mRunSockTest(
	connection.readArray( readbuf, writesize )==Network::Socket::ReadOK,
	"readArray after write & wait" );

    mRunSockTest( writebuf==readbuf,
	  "Returned data identical to sent data after write & wait");

    mRunSockTest(
	    connection.writeArray( writebuf.buf(), writesize, false ),
	    "writeArray & leave to echo server" );

    mRunSockTest(
	    connection.readArray(readbuf,writesize)==Network::Socket::ReadOK,
	    "readArray after write & leave" );

    mRunSockTest( writebuf==readbuf,
		  "Returned data identical to sent data after write & leave");

    mRunSockTest(
	connection.writeArray( writebuf.buf(), writesize, true ) &&
	connection.readArray( readbuf, writesize+1 )==Network::Socket::Timeout,
	"Reading more than available should timeout and fail" );

    BufferString readstring;

    mRunSockTest(
	    connection.write( writebuf ) &&
	    connection.read( readstring ) &&
	    readstring == writebuf,
	    "Sending and reading a string" );

    //The echo server cannot handle more than 2GB for some reason. Probably
    //because the buffers will overflow as we will not start reading
    //before everything is written.

    mRunSockTest(
	    connection.writeDoubleArray( doublewritearr, nrdoubles, false ),
	    "Write large array" );

    mRunSockTest( connection.readDoubleArray( doublereadarr, nrdoubles ),
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

    mRunSockTest( !readerror, "Large array integrity" );

    if ( closeserver )
	connection.write( BufferString(Network::Server::sKeyKillword()) );

    return true;
}


// Should operate against a server that echos all input back to sender, which
// can be specified by --serverapp "application". If no serverapp is given,
// echoserver is started

int mTestMainFnName(int argc, char** argv)
{
    mInitTestProg();
    ApplicationData app;
    clParser().setKeyHasValue( "serverapp" );
    clParser().setKeyHasValue( "timeout" );
    clParser().setKeyHasValue( Network::Server::sKeyPort() );

    PtrMan<TestRunner> runner = new TestRunner;
    runner->prefix_ = "[ No event loop ]\t";
    runner->exitonfinish_ = false;
    runner->noeventloop_ = true;

    BufferString serverapp = "test_echoserver";
    clParser().getKeyedInfo( "serverapp", serverapp, true );
    runner->servercmd_.setProgram( serverapp );

    runner->port_ = 1025;
    clParser().getKeyedInfo( "timeout", runner->timeout_, true );
    runner->servercmd_.addKeyedArg( "timeout", runner->timeout_ );

    runner->timeout_ = 600;
    clParser().getKeyedInfo( Network::Server::sKeyPort(), runner->port_, true );
    runner->servercmd_.addKeyedArg( Network::Server::sKeyPort(), runner->port_);

    if ( clParser().hasKey("quiet") )
	runner->servercmd_.addFlag( "quiet" );

    od_int64 totalmem, freemem;
    OD::getSystemMemory( totalmem, freemem );

    nrdoubles = freemem/sizeof(double);
    nrdoubles /= 4;
    nrdoubles /= 2; //Leave mem for OS and others
    nrdoubles = mMIN(nrdoubles,200000000); //1.6GB
    nrdoubles = mMAX(nrdoubles,2000); //Gotta send something

    doublewritearr = new double[nrdoubles];
    doublereadarr = new double[nrdoubles];

    MemSetter<double> memsetter( doublewritearr, 0, nrdoubles );
    memsetter.setValueFunc( &randVal );
    memsetter.execute();

    if ( !runner->testNetSocket() )
	return 1;

    //Now with a running event loop

    runner->prefix_ = "[ With event loop ]\t";
    runner->exitonfinish_ = true;
    runner->noeventloop_ = false;
    CallBack::addToMainThread( mCB(runner,TestRunner,testCallBack) );
    const int res = app.exec();

    runner = 0;

    return res;
}
