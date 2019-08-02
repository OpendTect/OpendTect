#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Payraudeau
 Date:          September 2005
________________________________________________________________________

-*/

#include "emcommon.h"
#include "sets.h"
#include "ranges.h"
#include "geomid.h"

class RowCol;
class od_ostream;
class BinnedValueSet;
class DataPointSet;
class TrcKeySampling;
class BufferStringSet;
namespace Pos { class Provider; }

namespace EM
{

class Surface;

/*!\brief Group of utilities for horizons: here are all functions required in
od_process_attrib_em for computing data on, along or between 2 horizons. */

mExpClass(EarthModel) HorizonUtils
{
public:
			HorizonUtils()		{}

    static float	getZ(const RowCol&,const Surface*);
    static float	getMissingZ(const RowCol&,const Surface*,int);
    static Surface*	getSurface(const DBKey&);
    static void		getPositions(od_ostream&,const DBKey&,
				     ObjectSet<BinnedValueSet>&);
    static void		getExactCoords(od_ostream&,const DBKey&,
					Pos::GeomID,const TrcKeySampling&,
					ObjectSet<DataPointSet>&);
    static void		getWantedPositions(od_ostream&,DBKeySet&,
					BinnedValueSet&,const TrcKeySampling&,
					const Interval<float>& extraz,
					int nrinterpsamp,int mainhoridx,
					float extrawidth,
					Pos::Provider* provider=0);
    static void		getWantedPos2D(od_ostream&,DBKeySet&,
					DataPointSet*,const TrcKeySampling&,
					const Interval<float>& extraz,
					Pos::GeomID);
    static bool		getZInterval(int idi,int idc,Surface*,Surface*,
					float& topz,float& botz,int nrinterpsmp,
					int mainhoridx,float& lastzinterval,
					float extrawidth);

    static bool		SolveIntersect(float& topz,float& botz,int nrinterpsamp,
					int is1main,float extrawidth,
					bool is1interp,bool is2interp);
    static void addSurfaceData(const DBKey&,const BufferStringSet&,
					const ObjectSet<BinnedValueSet>&);

protected:

};

} // namespace EM
