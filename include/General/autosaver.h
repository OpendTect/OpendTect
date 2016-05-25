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
#include "uistring.h"
class IOStream;



namespace OD
{

class Saveable;
class AutoSaveMgr;


/*!\brief Object ready for auto-save by OD::AUTOSAVE().  */


mExpClass(General) AutoSaver : public Monitorable
{
public:

			AutoSaver(const Saveable&);
			AutoSaver(const AutoSaver&);
			~AutoSaver();
			mDeclMonitorableAssignment(AutoSaver);

    mImplSimpleMonitoredGetSet(inline,nrSecondsBetweenSaves,
				      setNrSecondsBetweenSaves,
				      int,nrclocksecondsbetweenautosaves_,0)

			// These functions can be called from any thread

    bool		isActive() const;
    bool		autoSaveNow() const	{ return doAutoSaveWork(true); }
    void		userSaveOccurred() const;

protected:

    const Saveable*	saver_;

    mutable DirtyCountType lastautosavedirtycount_;
    mutable IOStream*	lastautosaveioobj_;
    mutable int		autosavenr_;
    mutable int		nrclocksecondsbetweenautosaves_;
    mutable int		lastautosaveclockseconds_;
    mutable int		curclockseconds_;

    virtual void	initAutoSave() const;
    bool		doAutoSaveWork(bool forcesave) const;

private:

    void		removePrevAutoSaved() const;
    bool		needsAutoSaveAct(int) const;
    bool		autoSave() const;
    void		saverDelCB(CallBacker*);
    friend class	AutoSaveMgr;

};


/*!\brief Auto-save manager. Singleton class.

  Work is done in its own thread.
  Saveable will automatically be removed when they are deleted.

  */

mExpClass(General) AutoSaveMgr : public CallBacker
{ mODTextTranslationClass(AutoSaveMgr)
public:

    bool		isActive(bool bydefault=false) const;
    void		setActive(bool,bool makedefault=true);
    int			nrSecondsBetweenSaves() const;
    void		setNrSecondsBetweenSaves(int);

    void		add(AutoSaver*);

			// triggered from mgr's thread. CB obj is the saver.
    Notifier<AutoSaveMgr> saveDone;
    Notifier<AutoSaveMgr> saveFailed;

private:

			AutoSaveMgr();
			~AutoSaveMgr();

    ObjectSet<AutoSaver> savers_;
    mutable Threads::Lock lock_;
    Threads::Thread*	thread_;
    bool		appexits_;
    bool		active_;

    void		appExits(CallBacker*);
    void		saverDelCB(CallBacker*);
    void		survChg( CallBacker* )	    { setEmpty(); }

    void		go();
    static inline void	goCB(CallBacker*);

public:

    // Probably not useful 4 u:

    static AutoSaveMgr&	getInst();		    // use OD::AUTOSAVE()
    void		remove(AutoSaver*);	    // done 4 u on destruction
    void		setEmpty();		    // why would u?
    Threads::Thread&	thread()		    { return *thread_; }

};


mGlobal(General) inline AutoSaveMgr& AutoSaveMGR()
{ return AutoSaveMgr::getInst(); }


}; //namespace OD

#endif
