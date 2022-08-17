#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          Aug 2010
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
