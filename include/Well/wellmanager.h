#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		May 2001 / Mar 2016
________________________________________________________________________

-*/

#include "wellmod.h"
#include "saveablemanager.h"
#include "welldata.h"


namespace Well
{

/*!\brief Manages all stored Well::Data objects.

 If a well is not yet loaded, then it will be loaded by fetch().

*/

mExpClass(Well) Manager : public SaveableManager
{ mODTextTranslationClass(Well::Manager)
public:

    ConstRefMan<Data>	fetch(const ObjID&,uiRetVal&) const;
    RefMan<Data>	fetchForEdit(const ObjID&,uiRetVal&);
    ConstRefMan<Data>	fetch(const ObjID&) const;
    RefMan<Data>	fetchForEdit(const ObjID&);

    ObjID		getID(const Data&) const;

    uiRetVal		store(const Data&,const IOPar* ioobjpars=0) const;
			//!< uses name to decide whether to create or replace
    uiRetVal		store(const Data&,const ObjID&,
				const IOPar* ioobjpars=0) const;
    uiRetVal		save(const ObjID&) const;
    uiRetVal		save(const Data&) const;
    bool		needsSave(const ObjID&) const;
    bool		needsSave(const Data&) const;

    bool		isPolygon(const ObjID&) const;
    bool		hasCategory(const ObjID&,const char*) const;

			// Use MonitorLock when iterating
    ConstRefMan<Data>	get(IdxType) const;
    RefMan<Data>	getForEdit(IdxType);

protected:

			Manager();
			~Manager();

    virtual Saveable*	getSaver(const SharedObject&) const;

    template<class T> T	doFetch(const ObjID&,uiRetVal&) const;
    Data*		gtData(const ObjID&) const;

public:

    mDeclareSaveableManagerInstance(Manager);

};


/*!\brief access to the singleton Well Manager */
inline Manager& MGR()
{
    return Manager::getInstance();
}


mExpClass(Well) Saver : public Saveable
{ mODTextTranslationClass(Well::Saver)
public:

			Saver(const Data&);
			mDeclMonitorableAssignment(Saver);
			mDeclInstanceCreatedNotifierAccess(Saver);
			~Saver();

    ConstRefMan<Data>	wellData() const;
    void		setWellData(const Data&);

protected:

    virtual bool	doStore(const IOObj&) const;

};


} // namespace Pick
