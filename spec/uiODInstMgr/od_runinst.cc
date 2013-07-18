/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          Mar 2012
________________________________________________________________________

-*/
static const char* rcsID = "$Id: od_runinst.cc 7930 2013-05-31 11:40:13Z ranojay.sen@dgbes.com $";

#include "prog.h"

#include "odinst.h"

int main( int argc, char** argv )
{
    SetProgramArgs( argc, argv );
    ODInst::startInstManagement();
}
