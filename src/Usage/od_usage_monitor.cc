/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Mar 2009
-*/

static const char* rcsID = "$Id: od_usage_monitor.cc,v 1.3 2009/07/22 16:01:35 cvsbert Exp $";

#include "batchprog.h"
#include "odusgserver.h"
#include "iopar.h"


bool BatchProgram::go( std::ostream& strm )
{
    IOPar* usgpars = Usage::Server::getPars();
    if ( !usgpars || usgpars->isEmpty() ) return true;

    Usage::Server srv( usgpars, strm );
    return srv.go();
}
