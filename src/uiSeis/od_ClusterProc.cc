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

    StreamProvider spin( parfilenm );
    StreamData sdin = spin.makeIStream();
    if ( !sdin.usable() )
    {
	std::cerr << argv[0] << ": Cannot open parameter file" << std::endl;
	ExitProgram( 1 );
    }

    IOPar iop; iop.read( *sdin.istrm, sKey::Pars() );
    if ( iop.size() == 0 )
    {
	std::cerr << argv[0] << ": Invalid parameter file" << std::endl;
	ExitProgram( 1 );
    }

    sdin.close();

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

    std::cout << "Merging output ... " << std::endl;
    TextTaskRunner tr( std::cout );
    BufferString msg;
    const bool result = uiClusterProc::mergeOutput( iop, &tr, msg, withdelete );
    std::cout << msg << std::endl;
    ExitProgram( result ? 0 : 1 );
    return 0;
}
