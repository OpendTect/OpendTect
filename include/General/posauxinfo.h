#ifndef posauxinfo_h
#define posauxinfo_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		Jul 2002
 Contents:	PAuxiliary info on position
 RCS:		$Id: posauxinfo.h,v 1.4 2005-05-02 09:14:13 cvskris Exp $
________________________________________________________________________

-*/

#include <rcol2coord.h>
#include <basiccompinfo.h>
#include <scaler.h>
#include <sets.h>
class CubeSampling;


/*!\brief Auxiliray data possibly needed at location. Mostly a seismic thing. */

class PosAuxInfo
{
public:

		PosAuxInfo()		{ clear(); }

    void	clear()
		{
		    binid.inl = binid.crl = 0; coord.x = coord.y = 0;
		    startpos = offset = azimuth = 0;
		    pick = refpos = mUndefValue;
		}

    BinID	binid;
    Coord	coord;
    float	startpos;
    float	offset;
    float	azimuth;
    float	pick;
    float	refpos;

};


/*!\brief Selection of aux info at location.
	  Note that BinID is always selected */

class PosAuxInfoSelection
{
public:
		PosAuxInfoSelection()
		: startpos(false), coord(false)
		, offset(false), azimuth(false)
		, pick(false), refpos(false)	{}

		void setAll( bool yn )
		{ startpos = coord = offset = azimuth = pick = refpos = yn; }

    bool	startpos;
    bool	coord;
    bool	offset;
    bool	azimuth;
    bool	pick;
    bool	refpos;
};


#endif
