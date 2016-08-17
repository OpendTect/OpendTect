/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Raman Singh
 * DATE     : Aug 2016
-*/


#include "batchprog.h"
#include "moddepmgr.h"


bool BatchProgram::go( od_ostream& strm )
{
    OD::ModDeps().ensureLoaded( "AllNonUi" );
    return true;
}
