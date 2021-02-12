/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2007
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

    CommandLineParser clp;
    if ( clp.nrArgs()<1 )
	return 1;

    int typ = 0; //Default is info
    if ( clp.hasKey( "warn" ) )
	typ = 1;
    else if ( clp.hasKey( "err" ) )
	typ = 2;
    else if ( clp.hasKey( "ask" ))
	typ = 3;

    BufferStringSet normalargs;
    clp.getNormalArguments( normalargs );

    uiString msg;
    for ( int idx=0; idx<normalargs.size(); idx++ )
    {
	BufferString nextarg( normalargs[idx]->buf() );
	nextarg.replace( "-+-", "\n" );
	msg.appendPlainText( nextarg );
	if ( idx<normalargs.size()-1 )
	    msg.appendPlainText( " " );
    }
    if ( msg.isEmpty() )
    {
	if ( typ != 3 )
	{
	    od_cout() << clp.getExecutableName() << ": No message";
	    return 0;
	}
	msg = od_static_tr("main","Your answer").addMoreInfo(uiString::empty());
    }

    uiMain app;
    OD::ModDeps().ensureLoaded( "uiBase" );
    uiMsg& uimsg = gUiMsg();
    if ( typ == 0 )
	uimsg.message( msg );
    else if ( typ == 1 )
	uimsg.warning( msg );
    else if ( typ == 2 )
	uimsg.error( msg );
    else if ( typ == 3 )
	od_cout() << getYesNoString( uimsg.askGoOn(msg) ) << od_endl;

    return 0;
}
