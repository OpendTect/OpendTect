#ifndef emhorizonutils_h
#define emhorizonutils_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Payraudeau
 Date:          September 2005
 RCS:           $Id$
________________________________________________________________________

-*/

#include "earthmodelmod.h"

#include "emsurfaceiodata.h"
#include "emposid.h"
#include "multiid.h"
#include "sets.h"
#include "ranges.h"

class RowCol;
class od_ostream;
class BinIDValueSet;
class DataPointSet;
class TrcKeySampling;
namespace Pos { class Provider; }

namespace EM
{

class Surface;

/*!
\brief Group of utilities for horizons: here are all functions required in
od_process_attrib_em for computing data on, along or between 2 horizons.
*/

mExpClass(EarthModel) HorizonUtils
{
public:
			HorizonUtils()		{}

    static float 	getZ(const RowCol&,const Surface*);
    static float 	getMissingZ(const RowCol&,const Surface*,int);
    static Surface* 	getSurface(const MultiID&);
    static void 	getPositions(od_ostream&,const MultiID&,
				     ObjectSet<BinIDValueSet>&);
    static void 	getExactCoords(od_ostream&,const MultiID&,
				       Pos::GeomID,const TrcKeySampling&,
				       ObjectSet<DataPointSet>&);
    static void 	getWantedPositions(od_ostream&,ObjectSet<MultiID>&,
					   BinIDValueSet&,const TrcKeySampling&,
					   const Interval<float>& extraz,
					   int nrinterpsamp,int mainhoridx,
					   float extrawidth,
					   Pos::Provider* provider=0);
    static void 	getWantedPos2D(od_ostream&,ObjectSet<MultiID>&,
				       DataPointSet*,const TrcKeySampling&,
				       const Interval<float>& extraz,
				       Pos::GeomID);
    static bool		getZInterval(int idi,int idc,Surface*,Surface*,
	    			     float& topz,float& botz,int nrinterpsamp,
				     int mainhoridx,float& lastzinterval,
				     float extrawidth);

    static bool		SolveIntersect(float& topz,float& botz,int nrinterpsamp,
	    			       int is1main,float extrawidth,
				       bool is1interp,bool is2interp);
    static void 	addSurfaceData(const MultiID&,const BufferStringSet&,
				       const ObjectSet<BinIDValueSet>&);

protected:

};


mExpClass( EarthModel ) HorizonSelInfo
{
public:
			HorizonSelInfo( const MultiID& key )
			    : key_(key), emobjid_(-1)	    {}
    BufferString	name_;
    MultiID		key_;
    ObjectID		emobjid_;
    EM::SurfaceIOData	iodata_;

    static void		getAll(ObjectSet<HorizonSelInfo>&,bool is2d);
};


} // namespace EM

#endif
