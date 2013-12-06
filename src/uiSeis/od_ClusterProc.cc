/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman Singh
 Date:          April 2009
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "prog.h"
#include "uiclusterproc.h"
#include "uimain.h"

#include "commandlineparser.h"
#include "envvars.h"
#include "executor.h"
#include "file.h"
#include "filepath.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "plugins.h"
#include "survinfo.h"
#include "od_iostream.h"


#define mPrintHelpMsg \
    od_cout() << "Usage: " << argv[0] << " parfile [--dosubmit] [--nodelete]" \
	      << od_endl;

int main( int argc, char ** argv )
{
    SetProgramArgs( argc, argv );
    
    CommandLineParser parser;

    const bool withdelete = !parser.hasKey( "nodelete" );
    const bool dosubmit = parser.hasKey( "dosubmit" );
    BufferStringSet normalargs;
    parser.getNormalArguments( normalargs );
    
    if ( normalargs.isEmpty() )
    {
	mPrintHelpMsg;
	return ExitProgram( 1 );
    }
    
    const BufferString parfilenm = normalargs.last()->buf();
    od_istream strm( parfilenm );
    if ( !strm.isOK() )
    {
	od_cout() << argv[0] << ": Cannot open parameter file" << od_endl;
	return ExitProgram( 1 );
    }

    IOPar iop; iop.read( strm, sKey::Pars() );
    if ( iop.size() == 0 )
    {
	od_cout() << argv[0] << ": Invalid parameter file" << od_endl;
	return ExitProgram( 1 );
    }

    const char* res = iop.find( sKey::Survey() );
    if ( res && *res && SI().getDirName() != res )
	IOMan::setSurvey( res );

    PIM().loadAuto( false );

    if ( dosubmit )
    {
	uiMain app( argc, argv );
	uiClusterProc* cp = new uiClusterProc( 0, iop );

	app.setTopLevel( cp );
	cp->show();

	int ret = app.exec();
	return ExitProgram( ret );
    }

    od_cout() << "Merging output ..." << od_endl;
    TextTaskRunner tr( od_cout() );
    BufferString msg;
    const bool result = uiClusterProc::mergeOutput( iop, &tr, msg, withdelete );
    od_cout() << msg << od_endl;

    return ExitProgram( result ? 0 : 1 );
}
