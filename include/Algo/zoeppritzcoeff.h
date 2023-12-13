#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "algomod.h"
#include "odcomplex.h"

class ElasticLayer;

/*!
\brief Zoeppritz Coefficients.
*/

mExpClass(Algo) ZoeppritzCoeff
{
public:
			ZoeppritzCoeff();
			~ZoeppritzCoeff();

    void		setInterface(float p,const ElasticLayer& el_layer1,
				     const ElasticLayer& el_layer2);

    float_complex       getCoeff(bool down_in,bool down_out,
				 bool p_in,bool p_out) const;

protected:

    float_complex	pdn_pdn_	= float_complex( 0.f, 0.f );
    float_complex	pdn_sdn_	= float_complex( 0.f, 0.f );
    float_complex	pdn_pup_	= float_complex( 0.f, 0.f );
    float_complex	pdn_sup_	= float_complex( 0.f, 0.f );

    float_complex	sdn_pdn_	= float_complex( 0.f, 0.f );
    float_complex	sdn_sdn_	= float_complex( 0.f, 0.f );
    float_complex	sdn_pup_	= float_complex( 0.f, 0.f );
    float_complex	sdn_sup_	= float_complex( 0.f, 0.f );

    float_complex	pup_pdn_	= float_complex( 0.f, 0.f );
    float_complex	pup_sdn_	= float_complex( 0.f, 0.f );
    float_complex	pup_pup_	= float_complex( 0.f, 0.f );
    float_complex	pup_sup_	= float_complex( 0.f, 0.f );

    float_complex	sup_pdn_	= float_complex( 0.f, 0.f );
    float_complex	sup_sdn_	= float_complex( 0.f, 0.f );
    float_complex	sup_pup_	= float_complex( 0.f, 0.f );
    float_complex	sup_sup_	= float_complex( 0.f, 0.f );
};


/*! Aki-Richard approx !*/
mGlobal(Algo) float_complex getFastCoeff(float p,const ElasticLayer& el_layer1,
					 const ElasticLayer& el_layer2);
