/*+
________________________________________________________________________

    (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
    Author:        Bruno
    Date:          May 2011
________________________________________________________________________

-*/

static const char* rcsID = "$Id: elasticpropsel.cc,v 1.1 2011-05-17 08:09:27 cvsbruno Exp $";

#include "elasticpropsel.h"


#define mDefPVel 2000
#define mDefDen 2
#define mDefSVel 500

void ElasticPropSel::fill( ElasticProps& props )
{
    fillPVel( props );
    fillSVel( props );
    fillDen( props );
}


void ElasticPropSel::fillPVel( ElasticProps& props )
{
    float& pvel = props.pvel_;
    const float den = props.den_;
    const float ai = props.ai_;
    valStatus status = params_.pvelstat_;

    if ( status == FromVel )
    {
	pvel = props.svel_; //TODO formula
    }
    else if ( status == FromAI )
    {
	if ( mIsUdf( ai ) )
	    pvel = mDefPVel;
	else if ( mIsUdf( den ) || den == 0)
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
    const float pvel = props.pvel_;
    valStatus status = params_.svelstat_;

    if ( status == FromVel )
    {
	svel = pvel; //TODO formula
    }
    if ( mIsUdf( svel ) ) 
	svel = mDefSVel;
}



void ElasticPropSel::fillDen( ElasticProps& props )
{
    float& den = props.den_;
    const float pvel = props.pvel_;
    const float ai = props.ai_;
    valStatus status = params_.denstat_;

    if ( status == FromAI )
    {
	if ( mIsUdf( ai ) )
	    den = mDefPVel;
	else if ( mIsUdf( pvel ) || pvel == 0)
	    den = ai / mDefPVel;
	else
	    den = ai / pvel;
    }
    if ( mIsUdf( den ) ) 
	den = mDefDen;
}
