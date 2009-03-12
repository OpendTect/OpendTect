/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : Mar 2009
-*/

static const char* rcsID = "$Id: od_usage_monitor.cc,v 1.1 2009-03-12 15:51:31 cvsbert Exp $";

#include "batchprog.h"
#include "odusgserver.h"


bool BatchProgram::go( std::ostream& strm )
{
    Usage::Server srv( pars(), strm );
    return srv.go();
}
