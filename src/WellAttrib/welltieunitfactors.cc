/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "welltieunitfactors.h"
#include "unitofmeasure.h"
#include "welllog.h"

namespace WellTie
{

const char*  UnitFactors::getStdVelLabel()
{ return "m/s"; }

const char*  UnitFactors::getStdTimeLabel()
{ return "s"; }

const char*  UnitFactors::getStdSonLabel()
{ return "us/m";  }


double UnitFactors::getDenFactor( const Well::Log& denlog  ) const
{
    const UnitOfMeasure* uom = getUOM( denlog );
    return uom ? calcDensFactor( uom->symbol() ) : mUdf(double);
}


double UnitFactors::getVelFactor( const Well::Log& vellog, bool issonic ) const
{
    const UnitOfMeasure* uom = getUOM( vellog );
    const char* s = uom ? uom->symbol() : 0;
    return s ? (issonic ? calcSonicVelFactor(s) : calcVelFactor(s))
	     : mUdf(double);
}


const UnitOfMeasure* UnitFactors::getUOM( const Well::Log& log ) const
{
    return log.unitOfMeasure();
}


double UnitFactors::calcSonicVelFactor( const char* velunit ) const
{
    const UnitOfMeasure* um = UoMR().get( velunit );
    return um ? um->userValue( 1.0 ) : 0.001*mFromFeetFactorF;
}


double UnitFactors::calcVelFactor( const char* velunit ) const
{
    const UnitOfMeasure* um = UoMR().get( velunit );
    return um ? um->userValue( 1.0 ) : 1000/mFromFeetFactorF;
}


double UnitFactors::calcDensFactor( const char* densunit ) const
{
    const UnitOfMeasure* um = UoMR().get( densunit );
    return um ? um->userValue(1.0) : 1000;
}

} // namespace WellTie
