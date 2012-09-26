/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Feb 2009
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

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
    return uom ? calcDensFactor( uom->symbol() ) : 0;
}


double UnitFactors::getVelFactor( const Well::Log& vellog, bool issonic ) const
{
    const UnitOfMeasure* uom = getUOM( vellog );
    const char* s= uom ? uom->symbol() : 0;
    return s ? issonic ? calcSonicVelFactor( s ) : calcVelFactor( s ) : 0; 
}


const UnitOfMeasure* UnitFactors::getUOM( const Well::Log& log ) const
{ 
    return UnitOfMeasure::getGuessed( log.unitMeasLabel() );
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

}; //namespace WellTie
