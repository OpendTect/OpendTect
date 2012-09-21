#ifndef vistristripset_h
#define vistristripset_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id$
________________________________________________________________________


-*/

#include "visbasemod.h"
#include "visshape.h"

namespace visBase
{

mClass(visBase) TriangleStripSet : public IndexedShape
{
public:
    static TriangleStripSet*	create()
				mCreateDataObj( TriangleStripSet );
};
};


#endif

