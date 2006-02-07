#ifndef emhorizonutils_h
#define emhorizonutils_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene Payraudeau
 Date:          September 2005
 RCS:           $Id: emhorizonutils.h,v 1.5 2006-02-07 13:38:40 cvshelene Exp $
________________________________________________________________________

-*/

#include "sets.h"
#include "ranges.h"

class RowCol;
class MultiID;
class BinIDValueSet;
class HorSampling;
class BufferStringSet;

namespace EM
{

/*! \brief
Group of utilities for horizons : here are all functions required in 
process_attrib_em for computing data on, along or between 2 horizons.
*/

class Surface;

class HorizonUtils
{
public:
			HorizonUtils(){};
				
    static float 	getZ(const RowCol&,const Surface*);
    static float 	getMissingZ(const RowCol&,const Surface*,int);
    static Surface* 	getSurface(const MultiID&);
    static void 	getPositions(std::ostream&,const MultiID&,
				     ObjectSet<BinIDValueSet>&);
    static void 	getWantedPositions(std::ostream&,ObjectSet<MultiID>&,
					   BinIDValueSet&,const HorSampling&,
					   const Interval<float>&,int,int,
					   float);
    static bool		getZInterval(int,int,Surface*,Surface*,
	    			     float&,float&,int,int,float&,float);
	
    static bool		SolveIntersect(float&,float&,int,int,float,bool,bool);
    static void 	addSurfaceData(const MultiID&,const BufferStringSet&,
				       const ObjectSet<BinIDValueSet>&);

protected:

};

};//namespace

#endif
