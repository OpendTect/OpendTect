#ifndef posauxinfo_h
#define posauxinfo_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		Jul 2002
 Contents:	PAuxiliary info on position
 RCS:		$Id: posauxinfo.h,v 1.1 2002-07-24 17:08:12 bert Exp $
________________________________________________________________________

-*/

#include <binid2coord.h>
#include <basiccompinfo.h>
#include <scaler.h>
#include <sets.h>
class CubeSampling;


/*!\brief Auxiliray data possibly needed at location. Mostly a seismic thing. */

class PosAuxInfo
{
public:

		PosAuxInfo()
		: binid(0,0), startpos(0), coord(0,0)
		, offset(0), pick(mUndefValue), refpos(mUndefValue)	{}

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

    bool	startpos;
    bool	coord;
    bool	offset;
    bool	azimuth;
    bool	pick;
    bool	refpos;
};


#endif
