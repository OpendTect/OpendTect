/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H.Bril
 Date:          Mar 2002
 RCS:           $Id: od_main.cc,v 1.1 2003-12-20 13:24:05 bert Exp $
________________________________________________________________________

-*/


#include "prog.h"
#include "uiodmain.h"


int ODMain( int argc, char** argv )
{
    PIM().setArgs( argc, argv );
    PIM().loadAuto( false );
    uiODMain* odmain = new uiODMain(argc,argv);
    PIM().loadAuto( true );
    odmain->go();
    delete odmain;
    return 0;
}


#ifndef __shlib__
int main( int argc, char** argv )
{
    BufferString envarg("DTECT_ARGV0=");
    envarg += argv[0];
    putenv( envarg.buf() );

    return ODMain( argc, argv );
}
#endif
