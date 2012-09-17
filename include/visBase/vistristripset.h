#ifndef vistristripset_h
#define vistristripset_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vistristripset.h,v 1.12 2009/07/22 16:01:25 cvsbert Exp $
________________________________________________________________________


-*/

#include "visshape.h"

namespace visBase
{

mClass TriangleStripSet : public IndexedShape
{
public:
    static TriangleStripSet*	create()
				mCreateDataObj( TriangleStripSet );
};
};


#endif
