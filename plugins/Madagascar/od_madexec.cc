/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	R. K. Singh
 Date:		March 2008
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "batchprog.h"
#include "iopar.h"
#include "madprocexec.h"
#include "moddepmgr.h"

bool BatchProgram::initWork( od_ostream& strm )
{
    OD::ModDeps().ensureLoaded( "AttributeEngine" );
    return true;
}

bool BatchProgram::doWork( od_ostream& strm )
{
    ODMad::ProcExec exec( pars(), strm );
    if ( !exec.init() || !exec.execute() )
    {
	BufferString cmd = "od_DispMsg --err ";
	cmd += exec.errMsg().getFullString();
	system( cmd );
	return false;
    }

    return true;
}    
