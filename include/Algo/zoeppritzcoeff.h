#ifndef zoeppritzcoeff_h
#define zoeppritzcoeff_h

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Jan 2011
 RCS:		$Id: zoeppritzcoeff.h,v 1.2 2011-03-01 07:58:58 cvsbruno Exp $
________________________________________________________________________

*/

#include "odcomplex.h"

class AILayer;

mClass ZoeppritzCoeff 
{
public:
			ZoeppritzCoeff();


    void                setInterface(float p,int layer,
					const AILayer& p_layer1,
					const AILayer& p_layer2,
					const AILayer& slayer1,
					const AILayer& slayer2);

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

#endif
