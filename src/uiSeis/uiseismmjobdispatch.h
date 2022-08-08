#ifndef uiseismmjobdispatch_h
#define uiseismmjobdispatch_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2014
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


#endif
