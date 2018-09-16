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

#ifdef __msvc__
#include "winmain.h"
#endif


int main( int argc, char** argv )
{
    OD::SetRunContext( OD::UiProgCtxt );
    SetProgramArgs( argc, argv );
    uiMain app;

    auto& clp = app.commandLineParser();
    if ( clp.nrArgs()<1 )
	return ExitProgram( 1 );

    int typ = 0; //Default is info
    if ( clp.hasKey( "warn" ) )
	typ = 1;
    else if ( clp.hasKey( "err" ) )
	typ = 2;
    else if ( clp.hasKey( "ask" ))
	typ = 3;

    BufferStringSet normalargs;
    clp.getNormalArguments( normalargs );

    uiString msg = uiString::empty();
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
	    ExitProgram( 0 );
	}
	msg = od_static_tr("main","Your answer").addMoreInfo(uiString::empty());
    }

    uiMsg& uimsg = gUiMsg();
    if ( typ == 0 )
	uimsg.message( msg );
    else if ( typ == 1 )
	uimsg.warning( msg );
    else if ( typ == 2 )
	uimsg.error( msg );
    else if ( typ == 3 )
	od_cout() << getYesNoString( uimsg.askGoOn(msg) ) << od_endl;

    return ExitProgram( 0 );
}
