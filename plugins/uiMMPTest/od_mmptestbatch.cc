/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Raman Singh
 * DATE     : Aug 2016
-*/


#include "batchprog.h"
#include "jobcommunic.h"
#include "moddepmgr.h"


mLoad1Module("OD::ModDepMgr::sAllNonUI()")

bool BatchProgram::doWork( od_ostream& strm )
{
    strm << "Successfully running Diagnostic program on host ";
    strm << GetLocalHostName() << od_endl;
    comm_->setState( JobCommunic::Working );
    comm_->sendState();
    sleepSeconds( 2 );
    comm_->setState( JobCommunic::WrapUp );
    comm_->sendState();
    return true;
}
