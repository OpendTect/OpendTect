/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "prog.h"

#include "uimsg.h"
#include "uimain.h"
#include "uistrings.h"

#include "commandlineparser.h"
#include "moddepmgr.h"


int mProgMainFnName( int argc, char** argv )
{
    mInitProg( OD::UiProgCtxt )
    SetProgramArgs( argc, argv, false );
    uiMain app( argc, argv );

    OD::ModDeps().ensureLoaded( "General" );

    const CommandLineParser parser( argc, argv );
    if ( parser.nrArgs()<1 )
	return 1;

    PIM().loadAuto( false );
    OD::ModDeps().ensureLoaded( "uiBase" );
    PIM().loadAuto( true );

    int typ = 0; //Default is info
    if ( parser.hasKey("warn") )
	typ = 1;
    else if ( parser.hasKey("err") )
	typ = 2;
    else if ( parser.hasKey("ask") )
	typ = 3;

    BufferStringSet normalargs;
    parser.getNormalArguments( normalargs );

    uiString msg;
    for ( int idx=0; idx<normalargs.size(); idx++ )
    {
	BufferString nextarg( normalargs[idx]->buf() );
	nextarg.replace( "-+-", "\n" );
	msg.append( nextarg );
	if ( idx<normalargs.size()-1 )
	    msg.append( toUiString(" ") );
    }
    if ( msg.isEmpty() )
	msg = typ == 1 ? od_static_tr("main", "Be careful!")
	    : (typ ==2 ? od_static_tr("main", "Problem found!")
		       : od_static_tr("main", "Your answer:"));

    if ( typ == 0 )
	uiMSG().message( msg );
    else if ( typ == 1 )
	uiMSG().warning( msg );
    else if ( typ == 2 )
	uiMSG().error( msg );
    else if ( typ == 3 )
	od_cout() << getYesNoString(uiMSG().askGoOn(msg)) << od_endl;

    return 0;
}
