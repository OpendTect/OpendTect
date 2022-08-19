#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "mmbatchjobdispatch.h"


namespace Batch
{

class PreStackMMProgDef : public MMProgDef
{
public:
			PreStackMMProgDef() : MMProgDef("od_PreStackMMBatch") {}

    bool		isSuitedFor(const char*) const override;
    bool		canHandle(const JobSpec&) const override;
};

} // namespace Batch
