/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Oct 2007
 RCS:           $Id: od_DispMsg.cc,v 1.1 2007-10-03 08:37:52 cvsbert Exp $
________________________________________________________________________

-*/


#include "prog.h"
#include "uimsg.h"
#include "uimain.h"
#include <iostream>

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
	msg += argv[argidx];
	if ( argidx < argc-1 )
	    msg += " ";
    }

    if ( msg.isEmpty() )
	msg = typ == 1 ? "Be careful!" : "Problem found!";

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
