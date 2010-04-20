#ifndef gravblockcalc_h
#define gravblockcalc_h
/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Apr 2010
 RCS:		$Id: grav.h,v 1.1 2010-04-20 10:00:59 cvsbert Exp $
________________________________________________________________________

*/

#include "position.h"
class MultiID;


namespace Grav
{

extern const float G; //!< Newton's constant, in SI units (about 6.67e-11)
float Bouguer(float h,float rho=1);


class Block
{
public:

			Block(const Coord&,const Coord&,Interval<float> zrg);

    float		calcValue(const Coord3&,float rho=1) const;
    			// answer in m/s^2. *1e5 is mgal

protected:

    const Coord3	c0_;
    const float		dx_, dy_, dz_;

    float		prim(float,float,float) const;

};

} // namespace Grav


#endif
