#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seismod.h"
#include "coord.h"

/*!
\brief Stores offset and azimuth as an int, wich makes it easy to compare them
without having to think of epsilons when comparing.

The offset has a precision of 0.1 meter and have the range of -419430 and
419430 meters.  The azimuth has a about 1.5 bins per degree (511 bins per full
circle).
*/

mExpClass(Seis) OffsetAzimuth
{
public:
			OffsetAzimuth();
			OffsetAzimuth(float off,float azi);
			~OffsetAzimuth();

    bool		operator==(const OffsetAzimuth&) const;
    bool		operator!=(const OffsetAzimuth&) const;

    int			asInt() const;
    void		setFrom(int);

    float		offset() const;
    float		azimuth() const;
    bool		isAzimuthUndef() const;
    bool		isOffsetUndef() const;
    void		setOffset(float);
    void		setAzimuth(float);
    float		distanceTo(const OffsetAzimuth&,bool sq=false) const;
			/*!<\note If sq is true, the square distance is
			  returned. */

    Coord		srcRcvPos(const Coord& center,bool add=true) const;
			/*!<\returns center + (or - depending on variable add)
			    the object's  offset and azimuth. */

protected:

    int			offsetazi_;

};
