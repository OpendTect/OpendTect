/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H.Bril
 Date:          Mar 2002
 RCS:           $Id: od_main.cc,v 1.2 2003-12-28 16:10:23 bert Exp $
________________________________________________________________________

-*/


#include "prog.h"

extern int ODMain(int,char**);

int main( int argc, char** argv )
{
    BufferString envarg("DTECT_ARGV0=");
    envarg += argv[0];
    putenv( envarg.buf() );

    return ODMain( argc, argv );
}
