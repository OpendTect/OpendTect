/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "zoeppritzcoeff.h"
#include "ailayer.h"

#include "math2.h"


ZoeppritzCoeff::ZoeppritzCoeff()
    : pdn_pdn_( 0, 0 )
    , pdn_sdn_( 0, 0 )
    , pdn_pup_( 0, 0 )
    , pdn_sup_( 0, 0 )
    , sdn_pdn_( 0, 0 )
    , sdn_sdn_( 0, 0 )
    , sdn_pup_( 0, 0 )
    , sdn_sup_( 0, 0 )
    , pup_pdn_( 0, 0 )
    , pup_sdn_( 0, 0 )
    , pup_pup_( 0, 0 )
    , pup_sup_( 0, 0 )
    , sup_pdn_( 0, 0 )
    , sup_sdn_( 0, 0 )
    , sup_pup_( 0, 0 )
    , sup_sup_( 0, 0 )
{}


void ZoeppritzCoeff::setInterface( float p, const ElasticLayer& el1, 
					    const ElasticLayer& el2 ) 
{
    const float p2 = p * p; 
    const float pvel1 = el1.vel_;
    const float pvel2 = el2.vel_;
    float svel1 = el1.svel_;
    float svel2 = el2.svel_;
    
    const bool waterabove = mIsZero(svel1,mDefEps);	//Detect water
    const bool waterbelow = mIsZero(svel2,mDefEps);

    if ( waterabove ) svel1 = 0.1;	// Set small values to make eqns work
    if ( waterbelow ) svel2 = 0.1;

    const float l1s2 = svel1 * svel1;
    const float l2s2 = svel2 * svel2;
    const float l1p2 = pvel1 * pvel1;
    const float l2p2 = pvel2 * pvel2;

    const float_complex a = el2.den_ * (1 -  2 * l2s2 * p2) -
	      el1.den_ * (1 -  2 * l1s2 * p2);	
    const float_complex b = el2.den_ * (1 -  2 * l2s2 * p2) +
	      el1.den_ * 2 * l1s2 * p2;	
    const float_complex c = el1.den_ * (1 -  2 * l1s2 * p2) +
	      el2.den_ * 2 * l2s2 * p2;	
    const float_complex d = 2 * (el2.den_ * l2s2 - el1.den_ * l1s2);

    const float_complex pzi1 = Math::Sqrt( float_complex( 1.f/l1p2 - p2, 0) );
    const float_complex pzi2 = Math::Sqrt( float_complex( 1.f/l2p2 - p2, 0) );

    const float_complex pzj1 = Math::Sqrt( float_complex( 1.f/l1s2 - p2, 0) );
    const float_complex pzj2 = Math::Sqrt( float_complex( 1.f/l2s2 - p2, 0) );

    const float_complex ee = b * pzi1 + c * pzi2;
    const float_complex ff = b * pzj1 + c * pzj2;
    const float_complex gg = a - d * pzi1 * pzj2;
    const float_complex hh = a - d * pzi2 * pzj1;
    const float_complex dd = ee*ff + gg*hh * p2;

    const float f2 = (float)2;

    pdn_pup_ = ( (b*pzi1 - c*pzi2) * ff - 
				(a + d * pzi1 * pzj2) * hh * p2) / dd;
    pdn_pdn_ = 2 * el1.den_ * pzi1 * ff * pvel1/(pvel2*dd);

    pdn_sup_ = -f2 * pzi1 * ( a*b + c*d * pzi2*pzj2 ) 
				* p * pvel1 /(svel2 *dd);
    pdn_sdn_ = 2 * el1.den_ * pzi1 * hh * p * pvel1/(svel2*dd);

    sdn_pup_ = -f2 * pzj1 * (a*b + c*d * pzi2*pzj2) * p * svel1/(pvel1*dd);
    sdn_pdn_ = -2 * el1.den_ * pzj1 * gg * 
				p * svel1/(pvel2*dd);
    sdn_sup_ = -( (b*pzj1 - c*pzj2) * ee - 
				(a + d*pzi2*pzj1) * gg*p2) /dd;
    sdn_sdn_ = 2 * el1.den_ * pzj1 * ee * svel1/(svel2*dd);
    pup_pup_ = 2 * el2.den_ * pzi2 * ff * pvel2/(pvel1*dd);
    pup_pdn_ = -( ( b*pzi1 - c*pzi2 ) * ff + 
				(a + d*pzi2*pzj1) * gg * p2)/dd;
    pup_sup_ = -2 * el2.den_ * pzi2 * gg * p * pvel2/(svel1*dd);
    pup_sdn_ = f2 * pzi2 * ( a*c + b*d*pzi1*pzj1) 
				* p * pvel2/(svel2*dd);

    sup_pup_ = 2 * el2.den_ * pzj2 * hh * p * svel2/(pvel1*dd);
    sup_pdn_ = f2 * pzj2 * (a*c + b*d * pzi1 * pzj1)  
				* p * svel2/(pvel2*dd);
    sup_sup_ = 2 * el2.den_ * pzj2 * ee * svel2/(svel1*dd);
    sup_sdn_ = ( (b*pzj1 - c*pzj2) * ee + 
				(a + d*pzi1*pzj2) * hh*p2)/dd;

    if ( waterabove )
    {
	sdn_pdn_ = 0;
	sdn_sdn_ = 0;
	sdn_pup_ = 0;
	sdn_sup_ = 0;

	pdn_sup_ = 0;
	pup_sup_ = 0;
	sup_sup_ = 0;
    }

    if ( waterbelow )
    {
	sdn_pdn_ = 0;
	sdn_sdn_ = 0;
	sdn_pup_ = 0;
	sdn_sup_ = 0;

	pdn_sdn_ = 0;
	pup_sdn_ = 0;
	sup_sdn_ = 0;
    }
} 


float_complex ZoeppritzCoeff::getCoeff( bool downin, bool downout,
					bool pin, bool pout ) const
{
    if ( downin )
    {
	if ( pin )
	{
	    if ( downout )
	    {
		if ( pout )
		    return pdn_pdn_;

		return pdn_sdn_;
	    }

	    if ( pout )
		return pdn_pup_;

	    return pdn_sup_;
	}
	
	if ( downout )
	{
	    if ( pout )
		return sdn_pdn_;

	    return sdn_sdn_;
	}
	
	if ( pout )
	    return sdn_pup_;

	return sdn_sup_;
    }

    if ( pin )
    {
	if ( downout )
	{
	    if ( pout )
		return pup_pdn_;

	    return pup_sdn_;
	}

	if ( pout )
	    return pup_pup_;

	return pup_sup_;
    }

    if ( downout )
    {
	if ( pout )
	    return sup_pdn_;

	return sup_sdn_;
    }

    if ( pout )
	return sup_pup_;

    return sup_sup_;
}



float_complex getFastCoeff( float par, const ElasticLayer& el1, 
				       const ElasticLayer& el2 )
{
    const float vp1 = el1.vel_; 
    const float vp2 = el2.vel_;
    const float dp1 = el1.den_; 
    const float dp2 = el2.den_;

    const float vs1 = el1.svel_; 
    const float vs2 = el2.svel_;

    const float Vp = ( vp2 + vp1 ) / 2;
    const float Vs = ( vs1 + vs2 ) / 2;
    const float P = ( dp1 + dp2 ) / 2;

    const float DVp = vp2 - vp1;
    const float DVs = vs2 - vs1;
    const float Dp = dp2 - dp1;

    if ( ( DVs == 0 &&  Dp == 0 )  || Vp == 0 || Vs == 0  || P == 0 )
	return float_complex( 0, 0 );

    const float sinangle = par * vp1;
    const float angle = asin( sinangle );
    const float cos2i = cos( angle) * cos( angle );

    const float A = 0.5f * ( ( 1 - 4*Vs*Vs*par*par )*Dp/P );
    const float B = 0.5f / cos2i*DVp/Vp;
    const float C = -4*Vs*Vs*par*par*DVs/Vs;

    return float_complex( A + B + C , 0 );
}
