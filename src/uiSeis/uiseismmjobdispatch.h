#ifndef uiseismmjobdispatch_h
#define uiseismmjobdispatch_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________

-*/

#include "mmbatchjobdispatch.h"


namespace Batch
{

class SeisMMProgDef : public MMProgDef
{
public:
			SeisMMProgDef() : MMProgDef("od_SeisMMBatch")	{}
    virtual bool	isSuitedFor(const char*) const;
    virtual bool	canHandle(const JobSpec&) const;
};

} // namespace Batch


#endif
