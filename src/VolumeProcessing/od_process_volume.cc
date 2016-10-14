    /*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y.C. Liu / Bert
 * DATE     : April 2007 / Feb 2016
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "batchprog.h"

#include "jobcommunic.h"
#include "volprocchainoutput.h"
#include "moddepmgr.h"


bool BatchProgram::go( od_ostream& strm )
{
    OD::ModDeps().ensureLoaded( "VolumeProcessing" );
    OD::ModDeps().ensureLoaded( "Well" );

    VolProc::ChainOutput vco;
    vco.usePar( pars() );
    if ( comm_ )
    {
	comm_->setState( JobCommunic::Working );
	comm_->sendState();
    }

    if ( !vco.go(strm) )
	return false;

    return true;
}
