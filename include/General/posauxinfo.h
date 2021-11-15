#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		Jul 2002
 Contents:	Auxiliary info on position
________________________________________________________________________

-*/

#include "generalmod.h"
#include "position.h"


/*!\brief Auxiliary data possibly needed at location. Mostly a seismic thing. */

mExpClass(General) PosAuxInfo
{
public:

		PosAuxInfo(bool for2d);

    mDeprecated("Provide 2D info")
		PosAuxInfo();

    void	clear();
    void	set2D(bool yn);

    bool	is2D() const	{ return trckey_.is2D(); }

    TrcKey	trckey_;
    Coord	coord;
    float	startpos = 0.f;
    float	offset = 0.f;
    float	azimuth = 0.f;
    float	pick = mUdf(float);
    float	refnr = mUdf(float);

    mDeprecated("Use TrcKey") BinID&	binid;

};


/*!\brief Selection of aux info at location.
	  Note that BinID is always selected */

mExpClass(General) PosAuxInfoSelection
{
public:
		PosAuxInfoSelection()
		: startpos(false), coord(false)
		, offset(false), azimuth(false)
		, pick(false), refnr(false)	{}

		void setAll( bool yn )
		{ startpos = coord = offset = azimuth = pick = refnr = yn; }

    bool	startpos;
    bool	coord;
    bool	offset;
    bool	azimuth;
    bool	pick;
    bool	refnr;
};


