/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	R. K. Singh
 Date:		March 2008
________________________________________________________________________

-*/

#include "batchprog.h"
#include "madprocexec.h"
#include "moddepmgr.h"

mLoad1Module("AttributeEngine")
{
    ODMad::ProcExec exec( pars(), strm );
    if ( !exec.init() || !exec.execute() )
    {
	OD::DisplayErrorMessage( ::toString(exec.errMsg()) );
	return false;
    }

    return true;
}
