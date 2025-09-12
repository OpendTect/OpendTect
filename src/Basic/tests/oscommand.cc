/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "applicationdata.h"
#include "envvars.h"
#include "filepath.h"
#include "od_iostream.h"
#include "oddirs.h"
#include "oscommand.h"
#include "testprog.h"
#include "timer.h"

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

    void doStop( bool iserr )
    {
	retval_ = iserr ? 1 : 0;
	CallBack::addToMainThread( mCB(this,TestClass,closeTesterCB) );
    }

    void timerTick( CallBacker* )
    {
	mDetachCB( timer_.tick, TestClass::timerTick );
	bool res = false;
        if ( clParser().hasKey("testpipes") )
        {
	    res = testServer();
	}
	else
	{
	    res = testCmds() &&
		  testAllPipes() &&
		  runCommandWithSpace() &&
		  runCommandWithLongOutput();
	    if ( res )
	    {
		isblocking_ = false;
		testAllPipes();
		mAttachCB( timer_.tick, TestClass::timeoutCB );
		timer_.start( 10000, true );
		return;
	    }
        }

	doStop( !res );
    }

    void timeoutCB( CallBacker* )
    {
	mDetachCB( timer_.tick, TestClass::timeoutCB );
	errStream() << "Timeout for process running in background" << od_endl;
	doStop( true );
    }

    void closeTesterCB( CallBacker* )
    {
        ApplicationData::exit( retval_ );
    }

static bool testCmds()
{
    OS::MachineCommand machcomm;
    OS::CommandExecPars execpars( OS::Wait4Finish );
    if ( __iswin__ )
    {
	machcomm.setProgram( "cmd " ).addArg( "/c" ).addArg( "dir" );
    }
    else
	machcomm.setProgram( "ls" ).addFlag( "l", OS::OldStyle );

    mRunStandardTestWithError( machcomm.execute(execpars),
		      "OS::MachineCommand::execute wait4finish",
		       toString(machcomm.errorMsg()) );

    execpars.launchtype( OS::RunInBG ).txtbufstdout( true );
    mRunStandardTestWithError( machcomm.execute(execpars),
		     "OS::MachineCommand::execute not wait4finish",
		     toString(machcomm.errorMsg()) );

    if ( !__iswin__ )
    {
	OS::MachineCommand headmc( "head", "-10" );
	machcomm.addPipedCommand( headmc );
    }

    BufferString stdoutstr;
    mRunStandardTestWithError( machcomm.execute(stdoutstr),
			"OS::MachineCommand::execute read stdout",
			toString(machcomm.errorMsg()) );
    BufferStringSet alllines;
    alllines.unCat( stdoutstr.buf() );
    mRunStandardTestWithError( __iswin__ ? alllines.size() > 20
					 : alllines.size() == 10,
			     "OS::MachineCommand::execute stdout size",
			     toString(machcomm.errorMsg()) );

    return true;
}


void startedCB( CallBacker* cb )
{
    tstStream() << "Process has started" << od_endl;
    if ( isblocking_ )
	return;

    mDynamicCastGet( OS::CommandLauncher*, cl, cb );
    if ( !cb )
    {
	doStop( true );
	return;
    }

    RefMan<OD::Process> process = cl->process();
    if ( !process )
    {
	doStop( true );
	return;
    }

    mAttachCB( process->readyReadStandardOutput,
	       TestClass::readStandardOutputCB );
    mAttachCB( process->readyReadStandardError,
	       TestClass::readStandardErrorCB );
}


void stateChangedCB( CallBacker* cb )
{
    if ( !cb || !cb->isCapsule() )
	return;

    mCBCapsuleUnpack( OD::Process::State, state, cb );
    tstStream() << "Process current state: "
		<< OD::Process::toString( state ) << od_endl;
}


void errorOccurredCB( CallBacker* cb )
{
    if ( !cb || !cb->isCapsule() )
	return;

    mCBCapsuleUnpack( OD::Process::Error, error, cb );
    errStream() << "Process error: "
		<< OD::Process::toString( error ) << od_endl;
    doStop( true );
}


using procresobj = std::pair<int,OD::Process::ExitStatus>;

void procFinishedCB( CallBacker* cb )
{
    if ( !cb || !cb->isCapsule() )
	return;

    mCBCapsuleUnpack( procresobj, exitcaps, cb );
    tstStream() << "Process finished with exit code '" << exitcaps.first
		<< "' and exit status: "
		<< OD::Process::toString(exitcaps.second) << od_endl;
}


void readStandardOutputCB( CallBacker* cb )
{
    mDynamicCastGet(OD::Process*,process,cb)
    if ( !cb )
	return;

    if ( !testRetStdout(*process) )
	doStop( true );
    else if ( stdoutret_ && stderrret_ )
	doStop( false );
}


void readStandardErrorCB( CallBacker* cb )
{
    mDynamicCastGet(OD::Process*,process,cb)
	if ( !cb )
	    return;

    if ( !testRetStderr(*process) )
	doStop( true );
    else if ( stdoutret_ && stderrret_ )
	doStop( false );
}


bool testRetStdout( OD::Process& process )
{
    BufferString stdoutstr;
    bool newlinefound = false;
    mRunStandardTestWithError( process.getLine( stdoutstr, true, &newlinefound )
			       && stdoutstr==mGoodReply, "Standard output",
	BufferString("Received message: ").add( stdoutstr.buf() ) );

    stdoutret_ = true;
    return true;
}


bool testRetStderr( OD::Process& process )
{
    BufferString stderrstr;
    bool newlinefound = false;
    mRunStandardTestWithError( process.getLine( stderrstr, false, &newlinefound)
			       && stderrstr==mWrongReply, "Error output",
	BufferString( "Received message: ").add( stderrstr.buf() ) );

    stderrret_ = true;
    return true;
}


bool testAllPipes()
{
    OS::MachineCommand mc( GetFullExecutablePath() );
    mc.addFlag( "testpipes" );
    OS::CommandLauncher cl( mc );
    mAttachCB( cl.started, TestClass::startedCB );
    mAttachCB( cl.stateChanged, TestClass::stateChangedCB );
    mAttachCB( cl.errorOccurred, TestClass::errorOccurredCB );
    mAttachCB( cl.finished, TestClass::procFinishedCB );

    OS::CommandExecPars clpars( OS::RunInBG );
    clpars.txtbufstdout( true ).txtbufstderr( true );

    mRunStandardTestWithError( cl.execute(clpars), "Launching triple pipes",
			       toString(cl.errorMsg()) );
    RefMan<OD::Process> proc = cl.process();
    mRunStandardTest( proc, "Has process object" );
    mRunStandardTest( cl.processID(), "Launched process has valid PID" );
    mRunStandardTest( proc->isRunning(), "Process is running" );
    mRunStandardTest( cl.hasStdInput(), "Has stdin stream" );
    mRunStandardTest( cl.hasStdOutput(), "Has stdout stream" );
    mRunStandardTest( cl.hasStdError(), "Has stderr stream" );

    const BufferString msg( mGoodMessage, " " );
    cl.write( msg.str() );
    if ( !isblocking_ )
	return true;

    mRunStandardTest( proc->waitForBytesWritten(), "Bytes sent to process" );
    while( true )
    {
	if ( !proc->waitForReadRead(100) &&
	     proc->state() == OD::Process::State::NotRunning )
	    break;

	BufferString stdoutstr, stderrstr;
	if ( proc->getLine(stdoutstr,true) && !stdoutstr.isEmpty() )
	{
	    mRunStandardTestWithError( stdoutstr==mGoodReply,
				       "Standard output",
		BufferString("Received message: ").add( stdoutstr.buf() ) );
	}

	if ( proc->getLine(stderrstr,false) && !stderrstr.isEmpty() )
	{
	    mRunStandardTestWithError( stderrstr==mWrongReply,
				       "Standard error",
		BufferString("Received message: ").add( stderrstr.buf() ) );
	}
    }

    mRunStandardTest( cl.exitCode() == 0, "Process exit code" );
    mRunStandardTest( cl.exitStatus() == OD::Process::ExitStatus::NormalExit,
		      "Process exit status" );

    return true;
}


static bool runCommandWithSpace()
{
    FilePath scriptfp( GetScriptDir(), "script with space");
    if ( __iswin__ )
	scriptfp.setExtension( "cmd" );
    else
	scriptfp.setExtension( "sh" );

    OS::MachineCommand machcomm( scriptfp.fullPath() );
    mRunStandardTestWithError( machcomm.execute(), "Command with space",
			       toString(machcomm.errorMsg()) );

    return true;
}


static bool runCommandWithLongOutput()
{
    //Run a command that will cause overflow in the input buffer. Output
    //Should be 100% correct, meaning that no bytes have been skipped or
    //inserted.

    FilePath scriptfp( GetScriptDir(), "count_to_1000" );
    if ( __iswin__ )
	scriptfp.setExtension( "cmd" );
    else
	scriptfp.setExtension( "csh" );

    BufferString output;
    OS::MachineCommand machcomm( scriptfp.fullPath() );
    mRunStandardTestWithError( machcomm.execute(output),
			"Executing count_to_1000 script",
			toString(machcomm.errorMsg()) );
    mRunStandardTest( output.size() == 3892, "Output has expected size" );

    BufferStringSet parsedoutput;
    parsedoutput.unCat( output.buf() );
    bool res = parsedoutput.size() == 1000;
    for ( int idx=0; idx<1000; idx++ )
    {
	if ( parsedoutput.get(idx) != toString(idx+1) )
	{
	    res = false;
	    break;
	}
    }

    mRunStandardTest( res, "Correctly reading long input stream" );
    return true;
}


static bool testServer()
{
    od_istream& in = od_cin();

    BufferString input;
    in.getWord( input );

    od_ostream& out = od_cout();
    if ( input==mGoodMessage )
    {
	out << mGoodReply << od_endl;
	od_cerr() << mWrongReply << od_endl;
	return true;
    }

    out << "I received: " << input.buf() << od_endl;
    od_cerr() << "Error" << od_endl;
    return false;
}

Timer   timer_;
bool	isblocking_ = true;
bool	stdoutret_ = false;
bool	stderrret_ = false;
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
