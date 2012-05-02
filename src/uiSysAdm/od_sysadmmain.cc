/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H.Bril
 Date:          Jul 2006
________________________________________________________________________

-*/
static const char* mUnusedVar rcsID = "$Id: od_sysadmmain.cc,v 1.2 2012-05-02 11:53:55 cvskris Exp $";


#include "prog.h"
#include "genc.h"
#include "errh.h"
#include <iostream>

extern int ODSysAdmMain(int,char**);

int main( int argc, char** argv )
{
    od_putProgInfo( argc, argv );

    int ret = ODSysAdmMain( argc, argv );
    ExitProgram( ret );
}
