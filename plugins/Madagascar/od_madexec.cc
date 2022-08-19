/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "batchprog.h"
#include "madprocexec.h"
#include "moddepmgr.h"

mLoad1Module("AttributeEngine")

bool BatchProgram::doWork( od_ostream& strm )
{
    ODMad::ProcExec exec( pars(), strm );
    if ( !exec.init() || !exec.execute() )
    {
	OD::DisplayErrorMessage( ::toString(exec.errMsg()) );
	return false;
    }

    return true;
}
