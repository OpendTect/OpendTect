/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : Mar 2009
-*/

static const char* rcsID = "$Id: od_usage_monitor.cc,v 1.2 2009-06-30 15:23:47 cvsbert Exp $";

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
