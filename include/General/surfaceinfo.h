#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"
#include "bufstring.h"
#include "integerid.h"
#include "multiid.h"

/*!\brief Surface info name/attribname with an ID (usually the EM-ID). */

mExpClass(General) SurfaceInfo
{
public:
			SurfaceInfo(const char* nm,const MultiID& mi,
				    const VisID& =VisID::udf(),
				    const char* attr=nullptr);
    virtual		~SurfaceInfo();

    MultiID		multiid_;
    VisID		visid_;
    BufferString	name_;
    BufferString	attrnm_;

};
