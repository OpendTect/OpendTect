/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H.Bril
 Date:          Mar 2002
 RCS:           $Id: od_main.cc,v 1.3 2004-01-15 16:17:03 dgb Exp $
________________________________________________________________________

-*/


#include "prog.h"

extern int ODMain(int,char**);

int main( int argc, char** argv )
{
    return ODMain( argc, argv );
}
