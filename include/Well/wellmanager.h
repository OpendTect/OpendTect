#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		May 2001 / Mar 2016
________________________________________________________________________

-*/

#include "welldata.h"
#include "welllog.h"
#include "saveablemanager.h"
#include "perthreadrepos.h"
#include <bitset>


namespace Well
{

class Writer;


/*\brief Tells the Well Manager what you need to be loaded.

Notes:
* D2T is only included if survey Z is Time. If you need it anyway, use
	forceAddD2T().
* Adding Trck will cause Inf to be added too. Removing trck does not kick out
	Inf automatically.

*/

mExpClass(Well) LoadReqs
{
public:

			LoadReqs(bool addall=true);
			LoadReqs(SubObjType);
			LoadReqs(SubObjType,SubObjType);
			LoadReqs(SubObjType,SubObjType,SubObjType);
    static LoadReqs	All();
    bool		operator ==( const LoadReqs& oth ) const
						{ return reqs_ == oth.reqs_; }

    LoadReqs&		add(SubObjType);
    LoadReqs&		forceAddD2T();
    LoadReqs&		remove( SubObjType typ ) { reqs_[typ]=0; return *this; }
    void		setToAll()		{ *this = All(); }
    void		setEmpty()		{ reqs_.reset(); }
    void		include(const LoadReqs&);

    bool		includes( SubObjType typ ) const
						{ return reqs_[typ]; }
    bool		includes(const LoadReqs&) const;
    bool		isAll() const		{ return includes( All() ); }

protected:

    std::bitset<mWellNrSubObjTypes>		reqs_;

};


/*!\brief Manages all stored Well::Data objects.

 If a well is not yet loaded, then it will be loaded by fetch().

*/


mExpClass(Well) Manager : public SaveableManager
{ mODTextTranslationClass(Well::Manager)
public:

    typedef LoadReqs	LoadState;

    ConstRefMan<Data>	fetch(const ObjID&,LoadReqs reqs=LoadReqs()) const;
    RefMan<Data>	fetchForEdit(const ObjID&,LoadReqs r=LoadReqs()) const;
    ConstRefMan<Data>	fetch(const ObjID&,LoadReqs,uiRetVal&) const;
    RefMan<Data>	fetchForEdit(const ObjID&,LoadReqs,uiRetVal&) const;

    ObjID		getID(const Data&) const;
    ObjID		getIDByUWI(const char*) const;
			//<! getIDByName() is in base class

    uiRetVal		store(const Data&,const TaskRunnerProvider&,
			      const IOPar* ioobjpars=0) const;
			//!< uses name to decide whether to create or replace
    uiRetVal		store(const Data&,const ObjID&,
			      const TaskRunnerProvider&,
			      const IOPar* ioobjpars=0) const;
    uiRetVal		save(const ObjID&,const TaskRunnerProvider&) const;
    uiRetVal		save(const Data&,const TaskRunnerProvider&) const;
    bool		needsSave(const ObjID&) const;
    bool		needsSave(const Data&) const;

    void		getLogNames(const ObjID&,BufferStringSet&) const;
    void		getLogInfo(const ObjID&,ObjectSet<IOPar>&) const;
    void		getMarkers(const ObjID&,BufferStringSet& nms,
				   TypeSet<Color>&,TypeSet<float>&) const;
    void		getAllMarkerInfos(BufferStringSet& nms,
					  TypeSet<Color>&,
					  TypeSet<float>&) const;
    ConstRefMan<Log>	getLog(const ObjID&,const char* lognm) const;
    Coord		getMapLocation(const ObjID&) const;

			// Use MonitorLock when iterating
    ConstRefMan<Data>	get(idx_type) const;
    RefMan<Data>	getForEdit(idx_type);

    IOObj*		getIOObjByUWI(const char*) const;
    uiRetVal		saveLog(const ObjID&,const char*,
				bool setwellsaved=false) const;

protected:

			Manager();
			~Manager();

    virtual Saveable*	getSaver(const SharedObject&) const;

    template<class T> T	doFetch(const ObjID&,const LoadReqs&,uiRetVal&) const;
    bool		readReqData(const ObjID&,Data&,const LoadReqs&,
				    uiRetVal&) const;
    Data*		gtData(idx_type) const;

    virtual void	handleObjAdd();
    virtual void	handleObjDel(idx_type);

    mutable TypeSet<LoadState>				loadstates_;
    mutable PerThreadObjectRepository<LoadState>	curloadstate_;

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

    typedef LoadReqs	StoreReqs;

			Saver(const Data&);
			mDeclMonitorableAssignment(Saver);
			mDeclInstanceCreatedNotifierAccess(Saver);
			~Saver();

    ConstRefMan<Data>	wellData() const;
    void		setWellData(const Data&);

protected:

    virtual uiRetVal	doStore(const IOObj&,const TaskRunnerProvider&) const;

    mutable TypeSet<DirtyCounter>   lastsavedsubobjdirtycounts_;
    void		updateLastSavedSubObjDirtyCounts(const Data&) const;
    StoreReqs		getStoreReqs(const Data&) const;

};


} // namespace Well
