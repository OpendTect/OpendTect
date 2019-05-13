#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		July 2016
________________________________________________________________________

-*/

#include "saveable.h"
#include "objectset.h"
class ChangeRecorder;
class IOObjContext;
class TaskRunnerProvider;

/*!\brief Base class for managers of Saveable objects: loading and storing.

 Typical code for read:

 uiRetVal retval;
 ConstRefMan<MyObj> myobj = MyObjMGR().fetch( id, retval );
 if ( retval.isError() )
     { errmsg_ = retval; return false; }

 Typical code for write:

 RefMan<MyObj> myobj = new MyObj;
 fillObj( *myobj );
 uiRetVal retval = MyObjMGR().store( *myobj, id );
 if ( retval.isError() )
     { errmsg_ = retval; return false; }

 Subclasses need to provide a method that returns a Saveable for a SharedObject,
 and optionally one that returns a ChangeRecorder.

*/

mExpClass(General) SaveableManager : public NamedMonitorable
{ mODTextTranslationClass(SaveableManager)
public:

    typedef ::DBKey				ObjID;
    typedef ObjectSet<Saveable>::size_type	size_type;
    typedef size_type				idx_type;

			~SaveableManager();

    bool		nameExists(const char*) const;
    bool		canSave(const ObjID&) const;
    uiRetVal		save(const ObjID&,const TaskRunnerProvider&) const;
    uiRetVal		saveAs(const ObjID& curid,const ObjID& newid,
			       const TaskRunnerProvider&) const;
    bool		needsSave(const ObjID&) const;
    void		setJustSaved(const ObjID&) const;

    bool		isValidID(const ObjID&) const;
    bool		isLoaded(const char*) const;
    bool		isLoaded(const ObjID&) const;
    void		getAll(DBKeySet&,bool onlyloaded=false) const;
    ObjID		getIDByName(const char*) const;
    IOPar		getIOObjPars(const ObjID&) const;
    void		setIOObjPars(const ObjID&,const IOPar&) const;
    BufferString	nameOf(const ObjID&) const;

			// Use MonitorLock when iterating
    size_type		size() const;
    ObjID		getIDByIndex(idx_type) const;
    IOPar		getIOObjParsByIndex(idx_type) const;

    CNotifier<SaveableManager,ObjID>	ObjOrphaned;
    CNotifier<SaveableManager,ObjID>	UnsavedObjLastCall;
    CNotifier<SaveableManager,ObjID>	ShowRequested;
    CNotifier<SaveableManager,ObjID>	HideRequested;
    CNotifier<SaveableManager,ObjID>	VanishRequested;

    CNotifier<SaveableManager,ObjID>	ObjAdded;
	/*!< Rarely used. Do not delete the object in the callbacker function.
	  Use the setNoDelete() when you create a RefMan */

    void		clearChangeRecords(const ObjID&);
    void		getChangeInfo(const ObjID&,
				  uiString& undotxt,uiString& redotxt) const;
    bool		useChangeRecord(const ObjID&,bool forundo);

    enum DispOpt	{ Show, Hide, Vanish };
    void		displayRequest(const DBKey&,DispOpt=Show);

protected:

			SaveableManager(const IOObjContext&,bool autosv,
					bool tempobjsonly=false);

    virtual Saveable*	getSaver(const SharedObject&) const	= 0;
    virtual ChangeRecorder* getChangeRecorder(const SharedObject&) const
								{ return 0; }
    virtual void	addCBsToObj(const SharedObject&);
    virtual void	handleObjAdd()				{}
    virtual void	handleObjDel(idx_type)			{}

    ObjectSet<Saveable>	savers_;
    ObjectSet<ChangeRecorder> chgrecs_;
    const IOObjContext&	ctxt_;
    const bool		autosaveable_;
    const bool		tempobjsonly_;

			// to be called from public obj-type specific ones
    ObjID		getID(const SharedObject&) const;
    uiRetVal		store(const SharedObject&,const TaskRunnerProvider&,
			      const IOPar*) const;
    uiRetVal		store(const SharedObject&,const ObjID&,
			      const TaskRunnerProvider&,const IOPar*) const;
    uiRetVal		save(const SharedObject&,
			     const TaskRunnerProvider&) const;
    bool		needsSave(const SharedObject&) const;
    IOObj*		getIOObj(const DBKey&) const;
    IOObj*		getIOObjByName(const char*) const;

			// Tools for locked state
    void		setEmpty();
    idx_type		gtIdx(const char*) const;
    idx_type		gtIdx(const ObjID&) const;
    idx_type		gtIdx(const SharedObject&) const;
    SharedObject*	gtObj(idx_type) const;
    const Saveable*	saverFor(const ObjID&) const;
    uiRetVal		doSave(const ObjID&,const TaskRunnerProvider&) const;
    void		add(const SharedObject&,const ObjID&,AccessLocker&,
				const IOPar*,bool) const;

    void		dbmEntryRemovedCB(CallBacker*);
    void		survChgCB(CallBacker*);
    void		appExitCB(CallBacker*);
    void		objDelCB(CallBacker*);
    void		objChgCB(CallBacker*);
    bool		isPresent(const SharedObject&) const;

public:

    SaveableManager*	clone() const		{ return 0; }
    void		handleUnsavedLastCall();

			// intended only for dedicated loaders
    void		addNew(const SharedObject&,const ObjID&,
				const IOPar*,bool) const;

};


#define mDeclareSaveableManagerInstance(typ) \
    static typ&	getInstance(); \
    typ*	clone() const	{ return 0; } \
    mDeclInstanceCreatedNotifierAccess(typ)

#define mDefineSaveableManagerInstance(typ) \
mDefineInstanceCreatedNotifierAccess(typ) \
 \
static typ* theinst_ = 0; \
static Threads::Lock theinstcreatelock_(true); \
 \
typ& typ::getInstance() \
{ \
    if ( !theinst_ ) \
    { \
	Threads::Locker locker( theinstcreatelock_ ); \
	if ( !theinst_ ) \
	    theinst_ = new typ; \
    } \
    return *theinst_; \
}


#define mDefineStdFns(typ) \
    ObjID		getID( const char* nm ) const \
			{ return SaveableManager::getID(nm); } \
    ObjID		getID( const SharedObject& obj ) const \
			{ return SaveableManager::getID(obj); } \
    ObjID		getID(const typ&) const; \
    uiRetVal		store(const typ&,const TaskRunnerProvider&, \
			      const IOPar* ioobjpars=0) const; \
    uiRetVal		store(const typ&,const ObjID&, \
			      const TaskRunnerProvider&, \
			      const IOPar* p=0) const; \
    uiRetVal		save(const typ&,const TaskRunnerProvider&) const; \
    bool		needsSave(const typ&) const
