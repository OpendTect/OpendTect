/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H.Bril
 Date:          Mar 2002
 RCS:           $Id: od_main.cc,v 1.5 2004-05-06 14:20:14 macman Exp $
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
    int ret=ODMain( argc, argv );
    exitProgram(ret);
}
