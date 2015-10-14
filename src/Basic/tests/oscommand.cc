/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2012
 * FUNCTION :
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "filepath.h"
#include "ptrman.h"
#include "oddirs.h"
#include "od_iostream.h"
#include "oscommand.h"
#include "strmdata.h"
#include "separstr.h"
#include "strmprov.h"
#include "testprog.h"
#include "thread.h"

#include <iostream>

#define mWrongReply "What!"
#define mGoodReply "Yo!"
#define mGoodMessage "OpendTect_Rules"


#define mRetFail(s) { od_cout() << "Failed " << s << od_endl; }


static bool testCmds()
{
#ifdef __win__
    //TODO
#else

#   define mMkCmd(s) BufferString(s,redir)
    const BufferString redir( quiet ? " >/dev/null" : "| head -10" );
    mRunStandardTest( OS::ExecCommand(mMkCmd("ls -l"),OS::Wait4Finish),
		      "OS::ExecCommand wait4finish" );
    mRunStandardTest( OS::ExecCommand(mMkCmd("ls -l"),OS::RunInBG),
		     "OS::ExecCommand not wait4finish" );

#endif

    return true;
}


static bool testAllPipes()
{
    BufferString cmd( GetFullExecutablePath() );
    cmd.add( " --testpipes" );
    const OS::MachineCommand mc( cmd );
    OS::CommandLauncher cl( mc );
    OS::CommandExecPars cp( false );
    cp.launchtype( OS::RunInBG ).createstreams( true );

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
    FilePath scriptname(GetSoftwareDir(0), "testscripts", "script with space");
#ifdef __win__
    scriptname.setExtension( "cmd" );
#else
    scriptname.setExtension( "sh" );
#endif

    mRunStandardTest( ExecODProgram(scriptname.fullPath(), 0, OS::Wait4Finish),
		      "Command with space" );

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
    const FilePath scriptname(GetSoftwareDir(0),"testscripts", "count_to_1000.csh");
    BufferString output;
    OS::ExecCommand(scriptname.fullPath(), OS::Wait4Finish, &output );

    SeparString parsedoutput( output.buf(), '\n' );
    bool res = true;
    for ( int idx=0; idx<1000; idx++ )
    {
	if ( parsedoutput[idx]!=toString( idx+1 ) )
	{
	    res = false;
	    break;
	}
    }

    mRunStandardTest( res, "Correctly reading long input stream" );

    return true;
#endif
}


static void testServer()
{
    Threads::sleep( 0.5 );

    od_istream in ( std::cin );

    BufferString input;
    in.getWord( input );

    od_ostream out( std::cout );
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



int main( int argc, char** argv )
{
    DBG::turnOn( 0 ); //Turn off all debug-stuff as it screwes the pipes
    mInitTestProg();

    if ( clparser.hasKey( "testpipes" ) )
    {
	testServer();

	return 0;
    }

    if ( !testCmds() || !testAllPipes() || !runCommandWithSpace() ||
	 !runCommandWithLongOutput() )
	ExitProgram( 1 );

    return ExitProgram( 0 );
}

