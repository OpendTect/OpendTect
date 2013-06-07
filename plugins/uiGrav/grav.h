#ifndef gravblockcalc_h
#define gravblockcalc_h
/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Apr 2010
 RCS:		$Id: grav.h,v 1.2 2010/04/20 12:53:18 cvsbert Exp $
________________________________________________________________________

*/

#include "position.h"
class MultiID;


namespace Grav
{

extern const double G; //!< Newton's constant, in SI units (about 6.67e-11)
double Bouguer(double h,double rho=1);


class Block
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


#endif
