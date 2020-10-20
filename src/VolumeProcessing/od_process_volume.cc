    /*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y.C. Liu / Bert
 * DATE     : April 2007 / Feb 2016
-*/


#include "batchprog.h"

#include "volprocprocessor.h"
#include "moddepmgr.h"

mLoad2Modules("VolumeProcessing","Well")

bool BatchProgram::doWork( od_ostream& strm )
{
    VolProc::Processor proc( pars() );
    if ( !proc.run(strm,comm_) )
	return false;

    return true;
}
