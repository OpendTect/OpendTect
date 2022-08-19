#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visshape.h"


namespace visBase
{

mExpClass(visBase) Lines : public VertexShape
{
public:
    static Lines*		create()
				mCreateDataObj(Lines);
};

} // namespace visBase
