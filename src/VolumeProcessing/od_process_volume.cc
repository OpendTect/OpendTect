    /*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y.C. Liu / Bert
 * DATE     : April 2007 / Feb 2016
-*/


#include "batchprog.h"

#include "volprocprocessor.h"
#include "moddepmgr.h"


bool BatchProgram::go( od_ostream& strm )
{
    OD::ModDeps().ensureLoaded( "VolumeProcessing" );
    OD::ModDeps().ensureLoaded( "Well" );

    VolProc::Processor proc( pars() );
    if ( !proc.run(strm) )
	return false;

    return true;
}
