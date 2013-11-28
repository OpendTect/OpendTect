/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2012
 * FUNCTION :
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "oscommand.h"
#include "commandlineparser.h"
#include "keystrs.h"

#include <iostream>


#define mRetFail(s) { std::cout << "Failed " << s << std::endl; }


static bool testCmds( bool quiet )
{
#ifdef __win__
    return true;
#else
#   define mMkCmd(s) BufferString(s,redir)
    const BufferString redir( quiet ? " >/dev/null" : "| head -10" );
    if ( !ExecOSCmd(mMkCmd("ls -l"),true) )
	mRetFail( "ExecOSCmd inconsole" );
    if ( !ExecOSCmd(mMkCmd("ls -l"),false) )
	mRetFail( "ExecOSCmd not in console" );
    if ( !ExecODProgram(mMkCmd("od_glxinfo"),"") )
	mRetFail( "ExecODProgram" );
#endif

    return true;
}


int main( int narg, char** argv )
{
    od_init_test_program( narg, argv );
    const bool quiet = CommandLineParser().hasKey( sKey::Quiet() );

    if ( !testCmds(quiet) )
	ExitProgram( 1 );

    ExitProgram( 0 );
}
