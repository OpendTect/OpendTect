#ifndef posauxinfo_h
#define posauxinfo_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		Jul 2002
 Contents:	PAuxiliary info on position
 RCS:		$Id: posauxinfo.h,v 1.10 2012-08-03 13:00:24 cvskris Exp $
________________________________________________________________________

-*/

#include "generalmod.h"
#include "position.h"
class CubeSampling;


/*!\brief Auxiliray data possibly needed at location. Mostly a seismic thing. */

mClass(General) PosAuxInfo
{
public:

		PosAuxInfo()		{ clear(); }

    void	clear()
		{
		    binid.inl = binid.crl = 0; coord.x = coord.y = 0;
		    startpos = offset = azimuth = 0;
		    pick = refnr = mUdf(float);
		}

    BinID	binid;
    Coord	coord;
    float	startpos;
    float	offset;
    float	azimuth;
    float	pick;
    float	refnr;

};


/*!\brief Selection of aux info at location.
	  Note that BinID is always selected */

mClass(General) PosAuxInfoSelection
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


#endif

