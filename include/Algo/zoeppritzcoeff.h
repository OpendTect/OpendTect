#ifndef zoeppritzcoeff_h
#define zoeppritzcoeff_h

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Jan 2011
 RCS:		$Id: zoeppritzcoeff.h,v 1.6 2012-08-03 13:00:07 cvskris Exp $
________________________________________________________________________

*/

#include "algomod.h"
#include "odcomplex.h"

class ElasticLayer;

mClass(Algo) ZoeppritzCoeff 
{
public:
			ZoeppritzCoeff();


    virtual void 	setInterface(float p,
					const ElasticLayer& el_layer1,
					const ElasticLayer& el_layer2 );

    float_complex       getCoeff(bool din,bool dout,bool pin,bool pout) const;

protected:

    float_complex       pdn_pdn_;
    float_complex       pdn_sdn_;
    float_complex       pdn_pup_;
    float_complex       pdn_sup_;

    float_complex       sdn_pdn_;
    float_complex       sdn_sdn_;
    float_complex       sdn_pup_;
    float_complex       sdn_sup_;

    float_complex       pup_pdn_;
    float_complex       pup_sdn_;
    float_complex       pup_pup_;
    float_complex       pup_sup_;

    float_complex       sup_pdn_;
    float_complex       sup_sdn_;
    float_complex       sup_pup_;
    float_complex       sup_sup_;
};


/*! Aki-Richard approx !*/
mGlobal(Algo) float_complex getFastCoeff(float p, const ElasticLayer& el_layer1,
					    const ElasticLayer& el_layer2 );

#endif

