/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2007
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "uimsg.h"
#include "uimain.h"
#include <iostream>
#include "commandlineparser.h"

#ifdef __msvc__
#include "winmain.h"
#endif


int main( int argc, char** argv )
{
    SetProgramArgs( argc, argv );
    CommandLineParser parser;
    
    if ( parser.nrArgs()<1 )
	return 1;
    
    int typ = 0; //Default is info
    if ( parser.hasKey( "warn" ) )
    { typ = 1; }
    else if ( parser.hasKey( "err" ) )
    { typ = 2; }
    else if ( parser.hasKey( "ask" ))
    { typ = 3; }
    
    BufferStringSet normalargs;
    parser.getNormalArguments( normalargs );
    
    BufferString msg = "";
    for ( int idx=0; idx<normalargs.size(); idx++ )
    {
	BufferString nextarg( normalargs[idx]->buf() );
	replaceString( nextarg.buf(), "-+-", "\n" );
	msg += nextarg;
	if ( idx<normalargs.size()-1 )
	    msg += " ";
    }
  
    if ( msg.isEmpty() )
	msg = typ == 1 ? "Be careful!"
	    : (typ ==2 ? "Problem found!"
		       : "Your answer:");

    uiMain app( argc, argv );
    if ( typ == 0 )
	uiMSG().message( msg );
    else if ( typ == 1 )
	uiMSG().warning( msg );
    else if ( typ == 2 )
	uiMSG().error( msg );
    else if ( typ == 3 )
    {
	msg = getYesNoString( uiMSG().askGoOn(msg) );
	std::cout << msg << std::endl;
    }

    ExitProgram( 0 );
}
