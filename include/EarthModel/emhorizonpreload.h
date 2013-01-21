#ifndef emhorizonpreload_h
#define emhorizonpreload_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          Aug 2010
 RCS:           $Id$
________________________________________________________________________

-*/

#include "earthmodelmod.h"
#include "callback.h"

#include "bufstringset.h"
#include "typeset.h"

class MultiID;
class TaskRunner;

namespace EM
{

/*!
\ingroup EarthModel
\brief %Horizon preloader
*/

mExpClass(EarthModel) HorizonPreLoader : public CallBacker
{
public:
				HorizonPreLoader();
				~HorizonPreLoader();

    bool			load(const TypeSet<MultiID>&,TaskRunner* tr=0);
    const MultiID&		getMultiID(const char* name) const;
    const BufferStringSet&	getPreloadedNames() const
    				{ return loadednms_; }
    const char*			errorMsg() const	    { return errmsg_; }
    void			unload(const BufferStringSet& hornms);

protected:
    void		surveyChgCB(CallBacker*);

    TypeSet<MultiID>	loadedmids_;
    BufferStringSet	loadednms_;
    BufferString	errmsg_;
};


mGlobal(EarthModel) HorizonPreLoader& HPreL();

} //namespace EM

#endif

