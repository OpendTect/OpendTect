#ifndef uiprestackmmjobdispatch_h
#define uiprestackmmjobdispatch_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2014
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


#endif
