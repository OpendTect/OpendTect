#ifndef emhorizonpreload_h
#define emhorizonpreload_h

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
#include "dbkey.h"


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

    bool			load(const DBKeySet&,TaskRunner* tr=0);
    DBKey			getDBKey(const char* name) const;
    const DBKeySet&		getPreloadedIDs() const;
    const BufferStringSet&	getPreloadedNames() const;
    const char*			errorMsg() const	    { return errmsg_; }
    void			unload(const BufferStringSet& hornms);

protected:

    void		surveyChgCB(CallBacker*);

    DBKeySet		loadedmids_;
    BufferStringSet	loadednms_;
    BufferString	errmsg_;

};


mGlobal(EarthModel) HorizonPreLoader& HPreL();

} //namespace EM

#endif
