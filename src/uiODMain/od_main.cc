/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H.Bril
 Date:          Mar 2002
 RCS:           $Id: od_main.cc,v 1.6 2004-12-16 09:39:57 bert Exp $
________________________________________________________________________

-*/


#include "prog.h"
#include "genc.h"

// TODO : Is there a better way to force linking with attribute factory?
#ifdef __mac__
# include "attribfact.h"
#endif

extern int ODMain(int,char**);

int main( int argc, char** argv )
{
    od_putProgInfo( argc, argv );
    int ret = ODMain( argc, argv );
    exitProgram( ret );
}
