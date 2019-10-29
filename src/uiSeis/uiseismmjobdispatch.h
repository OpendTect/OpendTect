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
    virtual bool	isSuitedFor(const char*) const;
    virtual bool	canHandle(const JobSpec&) const;
    virtual bool	canResume(const JobSpec&) const;
    static uiString	sSeisMMProcDesc()
				{ return tr("Distribution Computing client"); }
};

} // namespace Batch


#endif
