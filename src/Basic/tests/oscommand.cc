/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "applicationdata.h"
#include "envvars.h"
#include "filepath.h"
#include "oddirs.h"
#include "od_iostream.h"
#include "oscommand.h"
#include "separstr.h"
#include "testprog.h"
#include "timer.h"

#include <iostream>

#define mWrongReply "What!"
#define mGoodReply "Yo!"
#define mGoodMessage "OpendTect_Rules"


#define mRetFail(s) { od_cout() << "Failed " << s << od_endl; }


class TestClass : public CallBacker
{
public:
    TestClass()
        : timer_( "starter" )
    {
        mAttachCB( timer_.tick, TestClass::timerTick );
        timer_.start( 0, true );
    }

    ~TestClass()
    {
        detachAllNotifiers();
        CallBack::removeFromThreadCalls( this );
    }

    void timerTick( CallBacker* )
    {
        if ( clParser().hasKey("testpipes") )
        {
            testServer();
            CallBack::addToMainThread( mCB(this,TestClass,closeTesterCB) );
            return;
        }

        retval_ = testCmds() && testAllPipes() && runCommandWithSpace() &&
            runCommandWithLongOutput() ? 0 : 1;
        CallBack::addToMainThread( mCB( this, TestClass, closeTesterCB ) );
    }

    void closeTesterCB( CallBacker* )
    {
        ApplicationData::exit( retval_ );
    }

static bool testCmds()
{
#ifdef __win__
    //TODO
#else

    OS::MachineCommand machcomm( "ls", "-l" );
    if ( quiet_ )
	machcomm.addFileRedirect( "/dev/null" );
    else
	machcomm.addPipe().addArg( "head" ).addArg( "-10" );
    mRunStandardTest( machcomm.execute(OS::Wait4Finish),
		      "OS::MachineCommand::execute wait4finish" );
    OS::CommandExecPars execpars( OS::RunInBG );
    execpars.createstreams( true );
    mRunStandardTest( machcomm.execute(execpars),
		     "OS::MachineCommand::execute not wait4finish" );
#endif

    return true;
}


static bool testAllPipes()
{
    OS::MachineCommand mc( GetFullExecutablePath() );
    mc.addFlag( "testpipes" );
    OS::CommandLauncher cl( mc );
    OS::CommandExecPars cp( OS::RunInBG );
    cp.createstreams( true );

    mRunStandardTest( cl.execute( cp ), "Launching triple pipes" );
    mRunStandardTest( cl.processID(), "Launched process has valid PID" );
    *cl.getStdInput() << mGoodMessage << " ";
    cl.getStdInput()->flush();

    BufferString stdoutput;
    for ( int idx=0; idx<10; idx++ )
    {
	Threads::sleep( 0.5 );
	cl.getStdOutput()->getWord( stdoutput );
	if ( !stdoutput.isEmpty() )
	    break;

        if ( cl.getStdOutput()->stdStream().bad() )
            break;

        if ( cl.getStdOutput()->stdStream().eof() )
        {
            cl.getStdOutput()->stdStream().clear();
        }

    }

    mRunStandardTestWithError( stdoutput==mGoodReply, "Standard output",
	    BufferString("Received message: ").add( stdoutput.buf() ) );

    BufferString erroutput;
    cl.getStdError()->getWord( erroutput );
    mRunStandardTestWithError( erroutput==mWrongReply, "Error output",
	    BufferString( "Received message: ").add( erroutput.buf() ) );

    return true;
}


static bool runCommandWithSpace()
{
    FilePath scriptfp( GetSoftwareDir(0), "testscripts",
			    "script with space");
#ifdef __win__
    scriptfp.setExtension( "cmd" );
#else
    scriptfp.setExtension( "sh" );
#endif

    OS::MachineCommand machcomm( scriptfp.fullPath() );
    mRunStandardTest( machcomm.execute(), "Command with space" );

    return true;
}


static bool runCommandWithLongOutput()
{
#ifdef __win__
    return true;
#else
    //Run a command that will cause overflow in the input buffer. Output
    //Should be 100% correct, meaning that no bytes have been skipped or
    //inserted.
    //
    const FilePath scriptfp( GetSoftwareDir(0), "testscripts",
				 "count_to_1000.csh" );
    BufferString output;
    OS::MachineCommand machcomm( scriptfp.fullPath() );
    machcomm.execute( output );

    SeparString parsedoutput( output.buf(), '\n' );
    bool res = true;
    for ( int idx=0; idx<1000; idx++ )
    {
	if ( parsedoutput[idx]!=toString( idx+1 ) )
	    { res = false; break; }
    }

    mRunStandardTest( res, "Correctly reading long input stream" );
    return true;
#endif
}


static void testServer()
{
    Threads::sleep( 0.5 );

    od_istream& in = od_cin();

    BufferString input;
    in.getWord( input );

    od_ostream& out = od_cout();
    if ( input==mGoodMessage )
    {
	out << mGoodReply << od_endl;
	od_cerr() << mWrongReply << od_endl;
    }
    else
    {
	out << "I received: " << input.buf() << od_endl;
	od_cerr() << "Error" << od_endl;
    }
}

Timer   timer_;
int     retval_ = -1;

};


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    // Debugging output screws up the command output, needs to be disabled:
    UnsetOSEnvVar( "DTECT_DEBUG" );

    ApplicationData app;

    TestClass tester;
    return app.exec();
}
