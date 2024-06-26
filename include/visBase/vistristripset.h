#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visbasemod.h"
#include "visshape.h"

namespace visBase
{

mExpClass(visBase) TriangleStripSet : public VertexShape
{
public:
    static RefMan<TriangleStripSet> create();
				mCreateDataObj(TriangleStripSet);

protected:
				~TriangleStripSet();
};

} // namespace visBase
