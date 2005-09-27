#ifndef emhorizonutils_h
#define emhorizonutils_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene Payraudeau
 Date:          September 2005
 RCS:           $Id: emhorizonutils.h,v 1.1 2005-09-27 09:34:54 cvshelene Exp $
________________________________________________________________________

-*/

#include "sets.h"

class BinID;
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
				
    static float 	getZ(const BinID&, const ObjectSet<BinIDValueSet>&);
    static Surface* 	getSurface(const MultiID&);
    static void 	getPositions(std::ostream&, const MultiID&,
				     ObjectSet<BinIDValueSet>&);
    static void 	getWantedPositions(std::ostream&, ObjectSet<MultiID>&,
					   BinIDValueSet&, const HorSampling&);
    static void 	addSurfaceData(const MultiID&, const BufferStringSet&,
				       const ObjectSet<BinIDValueSet>&);

protected:

};

};//namespace

#endif
