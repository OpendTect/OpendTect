/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/


#include "applicationdata.h"
#include "moddepmgr.h"
#include "netserver.h"
#include "netsocket.h"
#include "odsysmem.h"
#include "odmemory.h"
#include "oscommand.h"
#include "sighndl.h"
#include "statrand.h"
#include "testprog.h"



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

    TestRunner( const Network::Authority& auth )
        : authority_(auth)
    {}

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

    bool		noeventloop_ = true;
    Network::Authority	authority_;
    int			timeout_ = 600;
    const char*		prefix_;
    bool		exitonfinish_ = true;
};


bool TestRunner::testNetSocket( bool closeserver )
{
    Network::Socket connection( authority_.isLocal(), !noeventloop_ );
    connection.setTimeout( timeout_ );

    const Network::Authority nonexisting( "non_existing_host",
				mCast(PortNr_Type,20000) );
    mRunSockTest( !connection.connectToHost(nonexisting,true),
		  "Not connecting to non-existing host" );

    mRunSockTest( connection.connectToHost(authority_,true),
		  "Connect to echo server" );

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


static void terminateServer( const PID_Type pid )
{
    Threads::sleep( 0.1 );
    if ( pid < 1 || !isProcessAlive(pid) )
	return;

    errStream() << "Terminating zombie server with PID: " << pid << od_endl;
    SignalHandling::stopProcess( pid );
}


// Should operate against a server that echos all input back to sender, which
// can be specified by --serverapp "application". If no serverapp is given,
// echoserver is started

int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();
    ApplicationData app;
    OD::ModDeps().ensureLoaded( "Network" );

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

    TestRunner runner( auth );

    PID_Type serverpid = -1;
    if ( !clParser().hasKey("noechoapp") )
    {
        BufferString serverapp = "test_netsocketechoserver";
        clParser().setKeyHasValue( "serverapp" );
        clParser().getVal( "serverapp", serverapp, true );

        OS::MachineCommand mc( serverapp );
        auth.addTo( mc );
        mc.addKeyedArg( "timeout", runner.timeout_ );
        //if ( quiet_ )
	    mc.addFlag( sKey::Quiet() );

        const OS::CommandExecPars execpars( OS::RunInBG );
        OS::CommandLauncher cl( mc );
        if ( !cl.execute(execpars) )
        {
	    od_ostream& strm = errStream();
	    strm << "Cannot start " << mc.toString(&execpars);
	    strm << ": " << toString(cl.errorMsg()) << od_endl;
	    return 1;
	}

        Threads::sleep( 1 );
        serverpid = cl.processID();
        mRunStandardTest( (isProcessAlive(serverpid)),
		    BufferString( "Server started with PID: ", serverpid ) );
    }

    runner.prefix_ = "[ No event loop ]\t";
    runner.exitonfinish_ = false;
    runner.noeventloop_ = true;
    if ( !runner.testNetSocket(false) )
    {
	terminateServer( serverpid );
	return 1;
    }

    //Now with a running event loop

    runner.prefix_ = "[ With event loop ]\t";
    runner.exitonfinish_ = true;
    runner.noeventloop_ = false;
    CallBack::addToMainThread( mCB(&runner,TestRunner,testCallBack) );
    const int retval = app.exec();

    if ( serverpid > 0 )
    {
        Threads::sleep(1);
        mRunStandardTest( (!isProcessAlive(serverpid)),
			  "Server has been stopped" );
        terminateServer( serverpid );
    }

    return retval;
}
