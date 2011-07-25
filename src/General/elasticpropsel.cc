/*+
________________________________________________________________________

    (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
    Author:        Bruno
    Date:          May 2011
________________________________________________________________________

-*/

static const char* rcsID = "$Id: elasticpropsel.cc,v 1.3 2011-07-25 15:07:49 cvsbruno Exp $";

#include "elasticpropsel.h"
#include "math.h"
#include "mathexpression.h"



void ElasticFormulaRepository::addFormula( const char* nm, const char* expr )
{
    formulas_ += new ElasticFormula( nm, expr ); 
}


DenElasticFormulaRepository::DenElasticFormulaRepository()
{
    addFormula( "AI", "AI/Velocity" ); 
}


PVelElasticFormulaRepository::PVelElasticFormulaRepository()
{
    addFormula( "Sonic", "1/Sonic" );
    addFormula( "AI", "AI/Density" );
    addFormula( "S-Wave", "sqrt( a*SWave^2 + b )");
}


SVelElasticFormulaRepository::SVelElasticFormulaRepository()
{
    addFormula( "Shear Sonic", "1/ShearSonic" );
    addFormula( "P-Wave", "sqrt( a*PWave^2+ b )" );
}



void ElasticPropGen::guessInputFromProps( const PropertyRefSelection& pps )
{
    findDen( pps );
    findPVel( pps );
    findSVel( pps );
}


void ElasticPropGen::fill( AILayer& el, const float* vals, int sz )
{
    el.den_ = getDen( vals, sz );
    el.vel_ = getPVel( vals, sz );
}


void ElasticPropGen::fill( ElasticLayer& el, const float* vals, int sz )
{
    el.den_ = getDen( vals, sz );
    el.vel_ = getPVel( vals, sz );
    el.svel_ = getSVel( vals, sz );
}


float ElasticPropGen::getDen( const float* vals, int sz )
{
    return getVal( deninpdata_, vals, sz );
}


float ElasticPropGen::getPVel( const float* vals, int sz )
{
    return getVal( pvelinpdata_, vals, sz );
}


float ElasticPropGen::getSVel( const float* vals, int sz )  
{
    return getVal( svelinpdata_, vals, sz );
}


float ElasticPropGen::getVal( InpData& inp, const float* vals, int sz )
{
    MathExpression* expr = inp.expr_;
    const TypeSet<int>& selidxs = inp.selidxs_;

    if ( expr ) 
    {
	for ( int idx=0; idx<selidxs.size(); idx++ )
	    expr->setVariableValue( idx, vals[selidxs[idx]] );
	return expr->getValue();
    }
    return selidxs.size() ? vals[selidxs[0]] : mUdf( float );
}


void ElasticPropGen::findDen( const PropertyRefSelection& pps )
{
    const int denidx = findQuantity( pps, PropertyRef::Den );
    if ( denidx >= 0 ) 
	deninpdata_.selidxs_ += denidx;
}


void ElasticPropGen::findPVel( const PropertyRefSelection& pps )
{
    const int pvelidx = findQuantity( pps, PropertyRef::Vel );
    InpData& inpdata = pvelinpdata_;
    if ( pvelidx >= 0 ) 
	inpdata.selidxs_ += pvelidx;
    else 
    {
	const int sonidx = findQuantity( pps, PropertyRef::Son );
	if ( sonidx >= 0 )
	{
	}
    }
}


void ElasticPropGen::findSVel( const PropertyRefSelection& pps )
{
    const int svelidx = findQuantity( pps, PropertyRef::Vel );
    if ( svelidx >= 0 ) 
	svelinpdata_.selidxs_ += svelidx;
}


int ElasticPropGen::findQuantity( const PropertyRefSelection& pps,
					 const PropertyRef::StdType& stdtype )
{
    for ( int idx=0; idx<pps.size(); idx++ )
    {
	if ( pps[idx]->stdType() == stdtype )
	    return idx;
    }
    return -1;
}

