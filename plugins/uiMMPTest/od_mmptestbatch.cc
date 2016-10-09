/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Raman Singh
 * DATE     : Aug 2016
-*/


#include "batchprog.h"
#include "hostdata.h"
#include "jobcommunic.h"
#include "moddepmgr.h"


bool BatchProgram::go( od_ostream& strm )
{
    OD::ModDeps().ensureLoaded( OD::ModDepMgr::sAllNonUI() );
    strm << "Successfully running Diagnostic program on host ";
    strm << HostData::localHostName() << od_endl;
    comm_->setState( JobCommunic::Working );
    comm_->sendState();
    sleepSeconds( 2 );
    comm_->setState( JobCommunic::WrapUp );
    comm_->sendState();
    return true;
}
