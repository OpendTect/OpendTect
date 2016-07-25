#ifndef saveablemanager_h
#define saveablemanager_h

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

mExpClass(General) SaveableManager : public Monitorable
{ mODTextTranslationClass(SaveableManager)
public:

    typedef ::MultiID	ObjID;

			~SaveableManager();

    bool		nameExists(const char*) const;
    bool		canSave(const ObjID&) const;
    uiRetVal		save(const ObjID&) const;
    uiRetVal		saveAs(const ObjID& curid,const ObjID& newid) const;
    bool		needsSave(const ObjID&) const;
    void		setNoSaveNeeded(const ObjID&) const;

    bool		isLoaded(const char*) const;
    bool		isLoaded(const ObjID&) const;
    ObjID		getID(const char*) const;
    ObjID		getID(const SharedObject&) const;
    IOPar		getIOObjPars(const ObjID&) const;

			// Use MonitorLock when iterating
    int			size() const;
    ObjID		getID(int) const;
    IOPar		getIOObjPars(int) const;

    CNotifier<SaveableManager,ObjID>	ObjAdded;
    CNotifier<SaveableManager,ObjID>	ObjOrphaned;
    CNotifier<SaveableManager,ObjID>	UnsavedObjLastCall;
    CNotifier<SaveableManager,ObjID>	ShowRequested;
    CNotifier<SaveableManager,ObjID>	HideRequested;
    CNotifier<SaveableManager,ObjID>	VanishRequested;

    void		clearChangeRecords(const ObjID&);
    void		getChangeInfo(const ObjID&,
				  uiString& undotxt,uiString& redotxt) const;
    bool		useChangeRecord(const ObjID&,bool forundo);

    enum DispOpt	{ Show, Hide, Vanish };
    void		displayRequest(const MultiID&,DispOpt=Show);

protected:

			SaveableManager();

    virtual Saveable*	getSaver(const SharedObject&) const	= 0;
    virtual ChangeRecorder* getChangeRecorder(const SharedObject&) const
								{ return 0; }
    virtual void	addCBsToObj(const SharedObject&);

    ObjectSet<Saveable>		savers_;
    ObjectSet<ChangeRecorder>	chgrecs_;

			// to be called from public obj-type specific ones
    uiRetVal		store(const SharedObject&,const IOPar*) const;
    uiRetVal		store(const SharedObject&,const ObjID&,
			      const IOPar*) const;
    uiRetVal		save(const SharedObject&) const;
    bool		needsSave(const SharedObject&) const;

			// Tools for locked state
    void		setEmpty();
    int			gtIdx(const char*) const;
    int			gtIdx(const ObjID&) const;
    int			gtIdx(const SharedObject&) const;
    uiRetVal		doSave(const ObjID&) const;
    void		add(const SharedObject&,const ObjID&,const IOPar*,
			    bool) const;

    void		iomEntryRemovedCB(CallBacker*);
    void		survChgCB(CallBacker*);
    void		appExitCB(CallBacker*);
    void		objDelCB(CallBacker*);
    void		objChgCB(CallBacker*);

    const IOObjContext*	ctxt_;
    IOObj*		getIOObj(const char*) const;

public:

    SaveableManager*	clone() const		{ return 0; }
    void		handleUnsavedLastCall();

};


#endif
