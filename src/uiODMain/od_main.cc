/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H.Bril
 Date:          Mar 2002
 RCS:           $Id: od_main.cc,v 1.4 2004-01-21 10:21:28 dgb Exp $
________________________________________________________________________

-*/


#include "prog.h"
#include "genc.h"

extern int ODMain(int,char**);

int main( int argc, char** argv )
{
    int ret=ODMain( argc, argv );
    exitProgram(ret);
}
