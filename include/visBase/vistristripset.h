#ifndef vistristripset_h
#define vistristripset_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vistristripset.h,v 1.10 2003-11-07 12:21:54 bert Exp $
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
