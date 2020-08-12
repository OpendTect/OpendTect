#pragma once
/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Apr 2010
 RCS:		$Id$
________________________________________________________________________

*/

#include "coord.h"


namespace Grav
{

extern const double G; //!< Newton's constant, in SI units (about 6.67e-11)
double Bouguer(double h,double rho=1);


mClass(uiGrav) Block
{
public:

			Block(const Coord&,const Coord&,Interval<double> zrg);

    double		calcValue(const Coord3&,double rho=1) const;
    			// returned in m/s^2. *1e5 is mgal

protected:

    const Coord3	c0_;
    const double	dx_, dy_, dz_;

    double		prim(double,double,double) const;

};

} // namespace Grav


