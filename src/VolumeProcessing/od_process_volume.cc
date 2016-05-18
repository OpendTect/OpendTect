    /*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y.C. Liu / Bert
 * DATE     : April 2007 / Feb 2016
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "batchprog.h"

#include "jobcommunic.h"
#include "volprocprocessor.h"
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

    VolProc::Processor proc( pars() );
    if ( !proc.run(strm,comm_) )
    {
	if ( comm_ )
	{
	    comm_->setState( JobCommunic::JobError );
	    comm_->sendState();
	}
    }

    if ( comm_ )
    {
	comm_->setState( JobCommunic::Finished );
	comm_->sendState();
    }

    return true;
}
