/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Mar 2009
-*/

static const char* rcsID mUnusedVar = "$Id: od_usage_monitor.cc,v 1.6 2012-05-07 11:57:18 cvskris Exp $";

#include "batchprog.h"
#include "odusgserver.h"
#include "iopar.h"
#include "moddepmgr.h"

bool BatchProgram::go( std::ostream& strm )
{
    OD::ModDeps().ensureLoaded( "Usage" );
    
    IOPar* usgpars = Usage::Server::getPars();
    if ( !usgpars || usgpars->isEmpty() ) return true;

    Usage::Server srv( usgpars, strm );
    return srv.go();
}
