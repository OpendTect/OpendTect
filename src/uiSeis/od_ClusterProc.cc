/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Raman Singh
 Date:          April 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: od_ClusterProc.cc,v 1.1 2009-05-25 09:10:47 cvsraman Exp $";

#include "uiclusterproc.h"
#include "uimain.h"
#include "plugins.h"

#include "envvars.h"
#include "prog.h"
#include "strmprov.h"
#include "strmdata.h"
#include "iopar.h"
#include "filepath.h"
#include "keystrs.h"
#include <iostream>
#ifndef __msvc__
# include <unistd.h>
#endif


int main( int argc, char ** argv )
{
/*    od_putProgInfo( argc, argv );

    const int bgadd = argc > 1 && !strcmp(argv[1],"-bg") ? 1 : 0;
    if ( argc+bgadd < 3 )
    {
	std::cerr << "Usage: " << argv[0] << " program parfile" << std::endl;
	ExitProgram( 1 );
    }

    FilePath fp( argv[ 2 + bgadd ] );
    const BufferString parfnm( fp.fullPath() );
    StreamProvider spin( parfnm );
    StreamData sdin = spin.makeIStream();
    if ( !sdin.usable() )
    {
	std::cerr << argv[0] << ": Cannot open parameter file" << std::endl;
	ExitProgram( 1 );
    }
    IOPar iop; iop.read( *sdin.istrm, sKey::Pars );
    if ( iop.size() == 0 )
    {
	std::cerr << argv[0] << ": Invalid parameter file" << std::endl;
	ExitProgram( 1 );
    }
    sdin.close();

#if !defined( __mac__ ) && !defined( __win__ )
    if ( bgadd )
    {
	switch ( fork() )
	{
	case -1:
	    std::cerr << argv[0] << ": cannot fork: "
		      << errno_message() << std::endl;
	    ExitProgram( 1 );
	case 0:		break;
	default:	return 0;
	}
    }
#endif

    const char* res = iop.find( sKey::Survey );
    if ( res && *res && SI().getDirName() != res )
	IOMan::setSurvey( res );

    PIM().setArgs( argc, argv );
    PIM().loadAuto( false );
*/
    uiMain app( argc, argv );
    IOPar iop;
    iop.read( argv[1], sKey::Pars );
    uiClusterProc* cp = new uiClusterProc( 0, iop );

    app.setTopLevel( cp );
    cp->show();

    int res = app.exec();
std::cerr << "Application executed. Exiting now ..." << std::endl;
    ExitProgram( res );
    return 0;
}
