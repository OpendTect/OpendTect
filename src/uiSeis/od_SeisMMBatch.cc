/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          April 2002
 RCS:           $Id: od_SeisMMBatch.cc,v 1.18 2006-02-02 10:37:28 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiseismmproc.h"
#include "uimain.h"
#include "plugins.h"
#include "ioman.h"
#include "survinfo.h"

#include "prog.h"
#include "strmprov.h"
#include "strmdata.h"
#include "ioparlist.h"
#include "filepath.h"
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
    StreamProvider spin( fp.fullPath() );
    StreamData sdin = spin.makeIStream();
    if ( !sdin.usable() )
    {
	std::cerr << argv[0] << ": Cannot open parameter file" << std::endl;
	ExitProgram( 1 );
    }
    IOParList parlist( *sdin.istrm );
    if ( parlist.size() == 0 || parlist[0]->size() == 0 )
    {
	std::cerr << argv[0] << ": Invalid parameter file" << std::endl;
	ExitProgram( 1 );
    }
    sdin.close();

#ifndef __win__
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

    parlist.setFileName( fp.fullPath() );
    const char* res = parlist[0]->find( sKey::Survey );
    if ( res && *res && SI().name() != res )
	IOMan::setSurvey( res );

    PIM().setArgs( argc, argv );
    PIM().loadAuto( false );

    uiMain app( argc, argv );
    uiSeisMMProc* smmp = new uiSeisMMProc( 0, argv[1+bgadd], parlist );

    app.setTopLevel( smmp );
    smmp->show();

    ExitProgram( app.exec() ); return 0;
}
