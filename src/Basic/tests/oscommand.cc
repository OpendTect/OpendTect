/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2012
 * FUNCTION :
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "oscommand.h"
#include "testprog.h"
#include "od_iostream.h"
#include "strmdata.h"
#include "strmprov.h"
#include "ptrman.h"
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
    cp.launchtype( OS::RunInBG );

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
    }

    mRunStandardTestWithError( stdoutput==mGoodReply, "Standard output",
	    BufferString("Received message: ").add( stdoutput.buf() ) );

    BufferString erroutput;
    cl.getStdError()->getWord( erroutput );
    mRunStandardTestWithError( erroutput==mWrongReply, "Error output",
	    BufferString( "Received message: ").add( erroutput.buf() ) );

    return true;
}


static bool testPipeInput()
{
    FixedString message = "OpendTect rules";
    const BufferString command( "@echo ", message );
    StreamData streamdata = StreamProvider( command ).makeIStream();
    mRunStandardTest( streamdata.istrm,  "Creation of standard input stream");
    PtrMan<od_istream> istream = new od_istream(streamdata.istrm);

    BufferString streaminput;
    mRunStandardTest( istream->getAll( streaminput ) , "Read from pipe" );
    mRunStandardTest( streaminput==message, "Pipe content check" );

    return true;
}

/*
bool testPipeOutput()
{
    FixedString message = "OpendTect rules";

    BufferString command = "@";
#ifdef __win__
    command.add( "more" );
#else
    command.add( "cat" );
#endif

    command.add( " > " ).add( tmpfnm );
    StreamProvider prov( command );
    StreamData ostreamdata = prov.makeOStream();
    mRunStandardTest( ostreamdata.ostrm,  "Creation of standard output stream");
    PtrMan<od_ostream> ostream = new od_ostream(ostreamdata.ostrm);

    *ostream << message;
    ostream->close();

    ostream = 0; //Deletes everything
    Threads::sleep( 1 );

    od_istream istream( tmpfnm );
    mRunStandardTest( istream.isOK(), "Opening temporary file");
    BufferString streaminput;

    istream.getAll( streaminput );
    istream.close();
    File::remove( tmpfnm );

    mRunStandardTest( streaminput==message, "Pipe content check" );

    return true;
}
 */


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
    mInitTestProg();

    if ( clparser.hasKey( "testpipes" ) )
    {
	testServer();

	return 0;
    }

    if ( !testCmds() || !testAllPipes() || !testPipeInput() )
	ExitProgram( 1 );

    return ExitProgram( 0 );
}
