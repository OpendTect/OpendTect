#ifndef picksetmanager_h
#define picksetmanager_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		May 2001 / Mar 2016
________________________________________________________________________

-*/

#include "geometrymod.h"
#include "saveablemanager.h"
#include "pickset.h"


namespace Pick
{
class SetLoaderExec;


/*!\brief Manages all stored Pick::Set's.

 If a set is not yet loaded, then it will be loaded by fetch().

*/

mExpClass(Geometry) SetManager : public SaveableManager
{ mODTextTranslationClass(Pick::SetManager)
public:

    ConstRefMan<Set>	fetch(const ObjID&,uiRetVal&,
				    const char* category=0) const;
    RefMan<Set>		fetchForEdit(const ObjID&,uiRetVal&,
				     const char* category=0);
    ConstRefMan<Set>	fetch(const ObjID&) const;
    RefMan<Set>		fetchForEdit(const ObjID&);

    ObjID		getID(const Set&) const;

    uiRetVal		store(const Set&,const IOPar* ioobjpars=0) const;
			//!< uses name to decide whether to create or replace
    uiRetVal		store(const Set&,const ObjID&,
				const IOPar* ioobjpars=0) const;
    uiRetVal		save(const ObjID&) const;
    uiRetVal		save(const Set&) const;
    bool		needsSave(const ObjID&) const;
    bool		needsSave(const Set&) const;

    bool		isPolygon(const ObjID&) const;
    bool		hasCategory(const ObjID&,const char*) const;

			// Use MonitorLock when iterating
    ConstRefMan<Set>	get(IdxType) const;
    RefMan<Set>		getForEdit(IdxType);

protected:

			SetManager();
			~SetManager();

    virtual Saveable*	getSaver(const SharedObject&) const;
    virtual ChangeRecorder* getChangeRecorder(const SharedObject&) const;

    template<class T> T	doFetch(const ObjID&,uiRetVal&,const char* cat=0) const;
    Set*		gtSet(const ObjID&) const;

public:

    mDeclareSaveableManagerInstance(SetManager);
    friend class	SetLoaderExec;

};


/*!\brief access to the singleton Pick Set Manager */
inline SetManager& SetMGR()
{
    return SetManager::getInstance();
}

} // namespace Pick



#endif
