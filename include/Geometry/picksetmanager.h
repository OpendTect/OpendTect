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
#include "pickset.h"
#include "objectset.h"
#include "multiid.h"
#include "uistring.h"
class IOObj;
class IOObjContext;


namespace Pick
{
class Set;
class SetSaver;
class SetManager;
class SetLoaderExec;
class SetLocEvRecord;

/*!\brief access to the singleton Pick Set Manager */

inline SetManager& SetMGR();


/*!\brief Manages all stored Pick::Set's.

 If a set is not yet loaded, then it will be loaded by fetch().

 Undo events are kept for each Set separately.

 Typical code for read:

 uiRetVal retval;
 ConstRefMan<Pick::Set> ps = Pick::SetMGR().fetch( id, retval );
 if ( retval.isError() )
     { errmsg_ = retval; return false; }

 MonitorLock ml( *ps );
 for ( int idx=0; idx<ps->size(); idx++ )
 {
     const Coord pos = ps->getPos( idx );
     // etc.
 }
 ml.unlockNow();

 Typical code for write:

 RefMan<Pick::Set> ps = new Pick::Set;
 fillPS( *ps );
 uiRetVal retval = Pick::SetMGR().store( *ps, id );
 if ( retval.isError() )
     { errmsg_ = retval; return false; }

*/

mExpClass(Geometry) SetManager : public Monitorable
{ mODTextTranslationClass(Pick::SetManager)
public:

    typedef ::MultiID			SetID;
    typedef LocationChangeEvent		LocEvent;

    ConstRefMan<Set>	fetch(const SetID&,uiRetVal&,
				    const char* category=0) const;
    RefMan<Set>		fetchForEdit(const SetID&,uiRetVal&,
				     const char* category=0);
    ConstRefMan<Set>	fetch(const SetID&) const;
    RefMan<Set>		fetchForEdit(const SetID&);

    bool		nameExists(const char*) const;
    bool		canSave(const SetID&) const;
    uiRetVal		store(const Set&,const IOPar* ioobjpars=0) const;
			//!< uses name to decide whether to create or replace
    uiRetVal		store(const Set&,const SetID&,
				const IOPar* ioobjpars=0) const;
    uiRetVal		save(const Set&) const;
    uiRetVal		save(const SetID&) const;
    uiRetVal		saveAs(const SetID& curid,const SetID& newid) const;
    bool		needsSave(const Set&) const;
    bool		needsSave(const SetID&) const;
    void		setNoSaveNeeded(const SetID&) const;

    bool		isPolygon(const SetID&) const;
    bool		hasCategory(const SetID&,const char*) const;
    bool		isLoaded(const char*) const;
    bool		isLoaded(const SetID&) const;
    SetID		getID(const char*) const;
    SetID		getID(const Set&) const;
    IOPar		getIOObjPars(const SetID&) const;

			// Use MonitorLock when iterating
    int			size() const;
    ConstRefMan<Set>	get(int) const;
    RefMan<Set>		getForEdit(int);
    SetID		getID(int) const;
    IOPar		getIOObjPars(int) const;


    CNotifier<SetManager,SetID>	SetAdded;
    CNotifier<SetManager,SetID>	SetShowRequested;
    CNotifier<SetManager,SetID>	SetHideRequested;
    CNotifier<SetManager,SetID>	SetVanishRequested;
    CNotifier<SetManager,SetID>	UnsavedSetLastCall;

			// creation and destruction are recorded automagically
    void		pushLocEvent(const SetID&,const LocEvent&);
    LocEvent		popLocEvent(const SetID&);

    enum DispOpt	{ Show, Hide, Vanish };
    void		displayRequest(const MultiID&,DispOpt=Show);

protected:

    typedef TypeSet<LocEvent>	LocEvRecord;

				SetManager();
				~SetManager();

    ObjectSet<SetSaver>		savers_;
    ObjectSet<LocEvRecord>	locevrecs_;

			// Tools for locked state
    void		setEmpty();
    int			gtIdx(const char*) const;
    int			gtIdx(const SetID&) const;
    int			gtIdx(const Set&) const;
    Set*		gtSet(const SetID&) const;
    template<class RT,class ST> RT doFetch(const SetID&,uiRetVal&,
					   const char* cat=0) const;
    void		addCBsToSet(const Set&);
    void		addLocEv(const SetID&,const LocEvent&);
    uiRetVal		doSave(const SetID&) const;

    void		add(const Set&,const SetID&,const IOPar*,bool) const;

    void		iomEntryRemovedCB(CallBacker*);
    void		survChgCB(CallBacker*);
    void		appExitCB(CallBacker*);
    void		setDelCB(CallBacker*);
    void		setChgCB(CallBacker*);

    const IOObjContext&	ctxt_;
    IOObj*		getIOObj(const char*) const;

public:

    static SetManager&	getInstance();
    mDeclInstanceCreatedNotifierAccess(SetManager);
    SetManager*		clone() const		{ return 0; }
    void		handleUnsavedLastCall();
    friend class	SetLoaderExec;

};

} // namespace Pick


inline Pick::SetManager& Pick::SetMGR()
{
    return SetManager::getInstance();
}


#endif
