#ifndef saveable_h
#define saveable_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		April 2016
________________________________________________________________________

-*/

#include "generalmod.h"
#include "sharedobject.h"
#include "dbkey.h"
#include "iopar.h"
#include "uistring.h"
class IOObj;


/*!\brief Object that can be saved at any time. */

mExpClass(General) Saveable : public Monitorable
{ mODTextTranslationClass(Saveable)
public:

			Saveable(const SharedObject&);
			~Saveable();
			mDeclAbstractMonitorableAssignment(Saveable);

    const SharedObject* object() const;
    bool		objectAlive() const	{ return objectalive_; }
    void		setObject(const SharedObject&);

    mImplSimpleMonitoredGetSet(inline,key,setKey,DBKey,storekey_,0)
    mImplSimpleMonitoredGetSet(inline,ioObjPars,setIOObjPars,IOPar,ioobjpars_,0)
			// The pars will be merge()'d with the IOObj's current

    virtual bool	save() const;
    virtual bool	store(const IOObj&) const;
    uiString		errMsg() const		{ return errmsg_; }

    bool		needsSave() const;
    void		setNoSaveNeeded() const;

    DirtyCountType	lastSavedDirtyCount() const
			{ return lastsavedirtycount_; }
    DirtyCountType	curDirtyCount() const;

    static ChangeType	cSaveSucceededChangeType()	{ return 1; }
    static ChangeType	cSaveFailedChangeType()		{ return 2; }

protected:

    const SharedObject* object_;
    Threads::Atomic<bool> objectalive_;
    DBKey		storekey_;
    IOPar		ioobjpars_;
    mutable uiString	errmsg_;
    mutable Threads::Atomic<DirtyCountType> lastsavedirtycount_;

			// This function can be called from any thread
    virtual bool	doStore(const IOObj&) const	= 0;

private:

    void		attachCBToObj();
    void		detachCBFromObj();
    void		objDelCB(CallBacker*);

};


#endif
