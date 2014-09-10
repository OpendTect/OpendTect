/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "prestacktrimstatics.h"

#include "iopar.h"


namespace PreStack
{

TrimStatics::TrimStatics()
    : Processor(sFactoryKeyword())
{}


TrimStatics::~TrimStatics()
{
}


bool TrimStatics::prepareWork()
{
    if ( !Processor::prepareWork() )
	return false;

    return true;
}


void TrimStatics::fillPar( IOPar& par ) const
{
}


bool TrimStatics::usePar( const IOPar& par )
{
    return true;
}


bool TrimStatics::doWork( od_int64 start, od_int64 stop, int )
{
    return true;
}

} // namespace PreStack
