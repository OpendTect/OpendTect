#ifndef emhorizonutils_h
#define emhorizonutils_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene Payraudeau
 Date:          September 2005
 RCS:           $Id: emhorizonutils.h,v 1.4 2005-11-09 16:43:46 cvshelene Exp $
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
			~HorizonUtils(){};
				
    static float 	getZ(const RowCol&, const Surface*);
    static float 	getMissingZ(const RowCol&, const Surface*);
    static Surface* 	getSurface(const MultiID&);
    static void 	getPositions(std::ostream&, const MultiID&,
				     ObjectSet<BinIDValueSet>&);
    static void 	getWantedPositions(std::ostream&, ObjectSet<MultiID>&,
					   BinIDValueSet&, const HorSampling&,
					   const Interval<float>&);
    static void 	addSurfaceData(const MultiID&, const BufferStringSet&,
				       const ObjectSet<BinIDValueSet>&);

protected:

};

};//namespace

#endif
