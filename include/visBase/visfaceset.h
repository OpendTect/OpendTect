#ifndef visfaceset_h
#define visfaceset_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: visfaceset.h,v 1.1 2003-01-09 09:41:31 kristofer Exp $
________________________________________________________________________


-*/

#include "visshape.h"

namespace visBase
{

/*!\brief
Implementation of an Indexed face set. 
A shape is formed by setting the coord index to a sequence of positions, ended
by a -1. The shape will automaticly connect the first and the last position
in the sequence. Multiple face sets can be set by adding new sequences after
the first one.
*/


class FaceSet : public IndexedShape
{
public:
    static FaceSet*	create()
			mCreateDataObj( FaceSet );
};

};

#endif
