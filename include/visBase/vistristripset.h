#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
________________________________________________________________________


-*/

#include "visbasemod.h"
#include "visshape.h"

namespace visBase
{

mExpClass(visBase) TriangleStripSet : public VertexShape
{
public:
    static TriangleStripSet*	create()
				mCreateDataObj( TriangleStripSet );
};

} // namespace visBase
