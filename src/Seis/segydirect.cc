/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : Sep 2008
-*/

static const char* rcsID = "$Id: segydirect.cc,v 1.2 2008-09-18 14:55:52 cvsbert Exp $";

#include "segydirectdef.h"
#include "segyscanner.h"
#include "segytr.h"
#include "seistrc.h"
#include "strmprov.h"


SEGY::DirectDef::DirectDef( const char* fnm )
    : tr_(0)
    , curfidx_(-1)
{
    readFromFile( fnm );
}


SEGY::DirectDef::DirectDef( const SEGY::Scanner& sc )
    : tr_(0)
    , curfidx_(-1)
    , geom_(sc.geomType())
    , pars_(sc.pars())
{
}


SEGY::DirectDef::~DirectDef()
{
    delete tr_;
}


bool SEGY::DirectDef::readFromFile( const char* fnm )
{
    StreamData sd( StreamProvider(fnm).makeIStream() );
    if ( !sd.usable() )
	return false;
    return false;
}


bool SEGY::DirectDef::writeToFile( const char* fnm ) const
{
    StreamData sd( StreamProvider(fnm).makeOStream() );
    if ( !sd.usable() )
	return false;
    return false;
}
