/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman Singh
 Date:          April 2009
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

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
#include "prog.h"
#include "strmprov.h"
#include "strmdata.h"
#include "survinfo.h"
#include "od_iostream.h"

#include <iostream>

#define mPrintHelpMsg \
    std::cerr << "Usage: " << argv[0] << " parfile [--dosubmit] [--nodelete]" \
	    << std::endl;

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
	ExitProgram( 1 );
    }
    
    const BufferString parfilenm = normalargs.last()->buf();
    od_istream strm( parfilenm );
    if ( !strm.isOK() )
    {
	std::cerr << argv[0] << ": Cannot open parameter file" << std::endl;
	ExitProgram( 1 );
    }

    IOPar iop; iop.read( strm, sKey::Pars() );
    if ( iop.size() == 0 )
    {
	std::cerr << argv[0] << ": Invalid parameter file" << std::endl;
	ExitProgram( 1 );
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
	ExitProgram( ret );
    }

    od_ostream logstrm( std::cout );
    logstrm << "Merging output ..." << od_endl;
    TextTaskRunner tr( logstrm );
    BufferString msg;
    const bool result = uiClusterProc::mergeOutput( iop, &tr, msg, withdelete );
    logstrm << msg << od_endl;
    ExitProgram( result ? 0 : 1 );
    return 0;
}
