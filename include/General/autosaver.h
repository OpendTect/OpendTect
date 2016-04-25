#ifndef autosaver_h
#define autosaver_h

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
#include "uistring.h"
class Timer;
class IOObj;
class IOStream;


/*!\brief Object that with auto-save.

  You need to provide a fingerprint of your state when asked (prepare for this
  coming from another thread). If the fingerprint differs from previous time,
  you will be asked to store. The IOObj you get will be a modified version
  (always default local format). It will have a temporary IOObj ID.
  The actual fingerprint is up to you. Just make the chance of accidental
  match very low.

  You should tell the object when the user has (in some way) successfully
  saved the object, so next autosave can be postponed.

*/


namespace OD
{

class AutoSaveMgr;


mExpClass(General) AutoSaver : public Monitorable
{
public:

			AutoSaver(const Monitorable&);
			~AutoSaver();
    const Monitorable&	monitored() const		{ return obj_; }

    mImplSimpleMonitoredGetSet(inline,key,setKey,MultiID,key_)
    mImplSimpleMonitoredGetSet(inline,nrSeconds,setNrSeconds,int,nrseconds_)
    inline		mImplSimpleMonitoredGet(isFinished,bool,objdeleted_)

			// These functions can be called from any thread
    virtual BufferString getFingerPrint() const		= 0;
    virtual bool	store(const IOObj&) const	= 0;
    virtual void	remove(const IOObj&) const;

    void		userSaveOccurred();
    bool		saveNow()		{ return doWork(true); }
    uiString		errMsg() const		{ return errmsg_; }

protected:

    const Monitorable&	obj_;
    MultiID		key_;
    int			nrseconds_;
    bool		objdeleted_;

    BufferString	prevfingerprint_;
    IOStream*		prevstoreioobj_;
    int			savenr_;
    int			curclockseconds_;
    int			lastsaveclockseconds_;
    mutable uiString	errmsg_;

    bool		doWork(bool forcesave);

private:

    void		objDel(CallBacker*);
    void		removePrevStored();

    bool		act(int);
    friend class	AutoSaveMgr;

};


/*!\brief Auto-save manager. Singleton class. */

mExpClass(General) AutoSaveMgr : public CallBacker
{
public:

    void		add(AutoSaver*);

    static AutoSaveMgr&	getInst();

private:

			AutoSaveMgr();

    Timer*		timer_;
    ObjectSet<AutoSaver> savers_;

};


mGlobal(General) inline AutoSaveMgr& AUTOSAVE()
{ return AutoSaveMgr::getInst(); }


}; //namespace OD

#endif
