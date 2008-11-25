/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          April 2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id: od_SeisMMBatch.cc,v 1.23 2008-11-25 15:35:25 cvsbert Exp $";

#include "uiseismmproc.h"
#include "uimain.h"
#include "plugins.h"
#include "ioman.h"
#include "survinfo.h"

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
    od_putProgInfo( argc, argv );

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

    uiMain app( argc, argv );
    uiSeisMMProc* smmp = new uiSeisMMProc( 0, iop, argv[1+bgadd], parfnm );

    app.setTopLevel( smmp );
    smmp->show();

    ExitProgram( app.exec() ); return 0;
}
