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

class SeisMMProgDef : public MMProgDef
{ mODTextTranslationClass(SeisMMProgDef)
public:
			SeisMMProgDef() : MMProgDef("od_SeisMMBatch")	{}
    bool		isSuitedFor(const char*) const override;
    bool		canHandle(const JobSpec&) const override;
    bool		canResume(const JobSpec&) const override;
    static uiString	sSeisMMProcDesc()
				{ return tr("Distributed Computing Client"); }
};

} // namespace Batch
