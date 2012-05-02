/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	R. K. Singh
 Date:		March 2008
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: od_madexec.cc,v 1.19 2012-05-02 15:11:11 cvskris Exp $";

#include "batchprog.h"
#include "iopar.h"
#include "madprocexec.h"

bool BatchProgram::go( std::ostream& strm )
{
    ODMad::ProcExec exec( pars(), strm );
    if ( !exec.init() || !exec.execute() )
    {
	BufferString cmd = "od_DispMsg --err ";
	cmd += exec.errMsg();
	system( cmd );
	return false;
    }

    return true;
}    


