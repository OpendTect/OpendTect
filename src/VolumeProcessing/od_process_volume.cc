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

    if ( comm_ )
    {
	comm_->setState( JobCommunic::Working );
	comm_->sendState();
	comm_->setTimeBetweenMsgUpdates( 5000 );
    }

    PtrMan<VolProc::ChainOutput> vco = new VolProc::ChainOutput;
    vco->usePar( pars() );
    vco->setJobCommunicator( comm_ );
    if ( !vco->go(strm) )
    {
	if ( comm_ )
	{
	    comm_->setState( JobCommunic::JobError );
	    comm_->sendState();
	}

	return false;
    }

    vco = 0;
    if ( comm_ )
    {
	comm_->setState( JobCommunic::Finished );
	comm_->sendState();
    }

    return true;
}
