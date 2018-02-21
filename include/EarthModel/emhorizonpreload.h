#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          Aug 2010
________________________________________________________________________

-*/

#include "emcommon.h"
#include "callback.h"
#include "bufstringset.h"
#include "dbkey.h"


class TaskRunnerProvider;

namespace EM
{

/*!
\brief %Horizon preloader
*/

mExpClass(EarthModel) HorizonPreLoader : public CallBacker
{ mODTextTranslationClass(HorizonPreLoader)
public:
				HorizonPreLoader();
				~HorizonPreLoader();

    bool			load(const DBKeySet&,bool is2d,
				     const TaskRunnerProvider&);
    DBKey			getDBKey(const char* name) const;
    const DBKeySet&		getPreloadedIDs() const;
    const BufferStringSet&	getPreloadedNames() const;
    const uiString		errorMsg() const	    { return errmsg_; }
    void			unload(const BufferStringSet& hornms);

protected:

    void		surveyChgCB(CallBacker*);

    DBKeySet		loadedids_;
    BufferStringSet	loadednms_;
    uiString		errmsg_;

};


mGlobal(EarthModel) HorizonPreLoader& HPreL();

} //namespace EM
