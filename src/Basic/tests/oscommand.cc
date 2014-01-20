/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2012
 * FUNCTION :
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "oscommand.h"
#include "testprog.h"


#define mRetFail(s) { od_cout() << "Failed " << s << od_endl; }


static bool testCmds()
{
#ifdef __win__
    //TODO
#else
#   define mMkCmd(s) BufferString(s,redir)
    const BufferString redir( quiet ? " >/dev/null" : "| head -10" );
    if ( !OS::ExecCommand(mMkCmd("ls -l"),OS::Wait4Finish) )
	mRetFail( "OS::ExecCommand wait4finish" );
    if ( !OS::ExecCommand(mMkCmd("ls -l"),OS::RunInBG) )
	mRetFail( "OS::ExecCommand not wait4finish" );
#endif

    return true;
}


int main( int argc, char** argv )
{
    mInitTestProg();

    if ( !testCmds() )
	ExitProgram( 1 );

    return ExitProgram( 0 );
}
