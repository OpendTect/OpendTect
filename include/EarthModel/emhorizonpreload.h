#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "earthmodelmod.h"
#include "callback.h"

#include "bufstringset.h"
#include "typeset.h"

class TaskRunner;

namespace EM
{

/*!
\brief %Horizon preloader
*/

mExpClass(EarthModel) HorizonPreLoader : public CallBacker
{
public:
				HorizonPreLoader();
				~HorizonPreLoader();

    bool			load(const TypeSet<MultiID>&,TaskRunner* tr=0);
    const MultiID&		getMultiID(const char* name) const;
    const TypeSet<MultiID>&	getPreloadedIDs() const;
    const BufferStringSet&	getPreloadedNames() const;
    const char*			errorMsg() const	    { return errmsg_; }
    void			unload(const BufferStringSet& hornms);

protected:
    void		surveyChgCB(CallBacker*);

    TypeSet<MultiID>	loadedmids_;
    BufferStringSet	loadednms_;
    BufferString	errmsg_;
};


mGlobal(EarthModel) HorizonPreLoader& HPreL();

} // namespace EM
