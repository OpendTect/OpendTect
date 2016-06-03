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
#include "monitor.h"
#include "multiid.h"
#include "iopar.h"
#include "uistring.h"
class IOObj;


namespace OD
{


/*!\brief Object that can be saved at any time. */

mExpClass(General) Saveable : public Monitorable
{ mODTextTranslationClass(Saveable)
public:

			Saveable(const Monitorable&);
			Saveable(const Saveable&);
			~Saveable();
			mDeclAbstractMonitorableAssignment(Saveable);

    const Monitorable*	monitored() const;
    bool		monitoredAlive() const	{ return monitoredalive_; }
    void		setMonitored(const Monitorable&);

    mImplSimpleMonitoredGetSet(inline,key,setKey,MultiID,storekey_,0)
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

protected:

    const Monitorable*	monitored_;
    MultiID		storekey_;
    IOPar		ioobjpars_;
    mutable uiString	errmsg_;
    Threads::Atomic<bool> monitoredalive_;
    mutable Threads::Atomic<DirtyCountType> lastsavedirtycount_;

			// This function can be called from any thread
    virtual bool	doStore(const IOObj&) const	= 0;

private:

    void		attachCBToObj();
    void		detachCBFromObj();
    void		objDelCB(CallBacker*);

};


}; //namespace OD

#endif
