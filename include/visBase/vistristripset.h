#ifndef vistristripset_h
#define vistristripset_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vistristripset.h,v 1.9 2003-01-07 10:27:31 kristofer Exp $
________________________________________________________________________


-*/

#include "visshape.h"

namespace visBase
{

class TriangleStripSet : public IndexedShape
{
public:
    static TriangleStripSet*	create()
				mCreateDataObj( TriangleStripSet );
};
};


#endif
