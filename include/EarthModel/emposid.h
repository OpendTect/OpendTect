#ifndef emposid_h
#define emposid_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: emposid.h,v 1.2 2002-05-22 06:17:42 kristofer Exp $
________________________________________________________________________


-*/

#include "multiid.h"

namespace EarthModel
{

/*!\brief
Each position in the earthmodel has a position id. The position Id is a
64 bit integer. The first 16 bits gives the EarthModel object it belongs
to. The meaning of the latter bits are interpreted by that object.
*/

class PosID
{
public:
    unsigned long long int	subid;
    MultiID			objid;
};

}; // Namespace


#endif
