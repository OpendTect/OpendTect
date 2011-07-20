/*+
________________________________________________________________________

    (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
    Author:        Bruno
    Date:          May 2011
________________________________________________________________________

-*/

static const char* rcsID = "$Id: elasticpropsel.cc,v 1.2 2011-07-20 14:23:49 cvsbruno Exp $";

#include "elasticpropsel.h"
#include "math.h"


#define mDefPVel 2000
#define mDefDen 2
#define mDefSVel 500


void ElasticPropSel::fill( ElasticModel& model )
{
    for ( int idx=0; idx<model.size(); idx++ )
	fill( model[idx] );
}


void ElasticPropSel::fill( AIModel& model )
{
    for ( int idx=0; idx<model.size(); idx++ )
	fill( model[idx] );
}


void ElasticPropSel::fill( AILayer& layer )
{
    float dummysvel = mUdf(float);
    ElasticProps ep( layer.den_, layer.vel_, dummysvel );
    fill( ep );
}


void ElasticPropSel::fill( ElasticLayer& layer )
{
    ElasticProps ep( layer.den_, layer.vel_, layer.svel_ );
    fill( ep );
}


void ElasticPropSel::fill( ElasticProps& props )
{
    fillPVel( props );
    fillSVel( props );
    fillDen( props );
}


void ElasticPropSel::fillPVel( ElasticProps& props )
{
    float& pvel = props.pvel_;
    if ( !mIsUdf( pvel ) ) return;

    valStatus status = params_.pvelstat_;

    if ( status == FromVel )
    {
	const float svel = props.svel_;
	const float p2safac = params_.pvel2svelafac_;
	const float p2sbfac = params_.pvel2svelbfac_;
	pvel = sqrt( (svel*svel - p2sbfac)/p2safac );
    }
    else if ( status == FromAI  )
    {
	const float den = props.den_;
	const float ai = props.ai_;
	if ( mIsUdf( den ) || den == 0)
	    pvel = ai / mDefDen;
	else
	    pvel = ai / den;
    }

    if ( mIsUdf( pvel ) ) 
	pvel = mDefPVel;
}


void ElasticPropSel::fillSVel( ElasticProps& props  )
{
    float& svel = props.svel_;
    if ( !mIsUdf( svel ) ) return;

    valStatus status = params_.svelstat_;

    if ( status == FromVel )
    {
	const float pvel = props.pvel_;
	const float p2safac = params_.pvel2svelafac_;
	const float p2sbfac = params_.pvel2svelbfac_;
	svel = sqrt( p2safac*pvel*pvel + p2sbfac );
    }
    if ( mIsUdf( svel ) ) 
	svel = mDefSVel;
}



void ElasticPropSel::fillDen( ElasticProps& props )
{
    float& den = props.den_;
    if ( !mIsUdf(den) ) return;

    valStatus status = params_.denstat_;

    if ( status == FromAI ) 
    {
	const float pvel = props.pvel_;
	const float ai = props.ai_;
	if ( mIsUdf( pvel ) || pvel == 0)
	    den = ai / mDefPVel;
	else
	    den = ai / pvel;
    }
    if ( mIsUdf( den ) ) 
	den = mDefDen;
}
