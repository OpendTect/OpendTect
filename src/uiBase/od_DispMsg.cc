/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: od_DispMsg.cc,v 1.7 2009/07/22 16:01:37 cvsbert Exp $";


#include "uimsg.h"
#include "uimain.h"
#include <iostream>

#ifdef __msvc__
#include "winmain.h"
#endif


int main( int argc, char** argv )
{
    if ( argc < 2 ) return 1;

    BufferString msg( argv[1] );
    int typ = 0;
    int argidx = 1;
    if ( msg == "--info" )
	argidx++;
    else if ( msg == "--warn" )
	{ typ = 1; argidx++; }
    else if ( msg == "--err" )
	{ typ = 2; argidx++; }
    else if ( msg == "--ask" )
	{ typ = 3; argidx++; }

    msg = "";
    for ( ; argidx<argc; argidx++ )
    {
	BufferString nextarg( argv[argidx] );
	replaceString( nextarg.buf(), "-+-", "\n" );

	msg += nextarg;
	if ( argidx < argc-1 )
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
