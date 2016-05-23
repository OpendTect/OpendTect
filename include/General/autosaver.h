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
#include "saveable.h"
class IOStream;



namespace OD
{

class AutoSaveMgr;


/*!\brief Object ready for auto-save by OD::AUTOSAVE().

  just adds some variables and tools needed for the autosaving.
 */


mExpClass(General) AutoSaveable : public Saveable
{
public:

			AutoSaveable(const Monitorable&);
			AutoSaveable(const AutoSaveable&);
			~AutoSaveable();
			mDeclMonitorableAssignment(AutoSaveable);

    mImplSimpleMonitoredGetSet(inline,nrSecondsBetweenSaves,
				      setNrSecondsBetweenSaves,
				      int,nrclocksecondsbetweenautosaves_,0)

			// These functions can be called from any thread

    bool		autoSaveNow() const	{ return doAutoSaveWork(true); }
    void		userSaveOccurred() const;

    virtual bool	save() const;
    virtual bool	store(const IOObj&) const;
    virtual void	remove(const IOObj&) const;

protected:

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

    void		add(AutoSaveable*);

			// triggered from mgr's thread. CB obj is the saver.
    Notifier<AutoSaveMgr> saveDone;
    Notifier<AutoSaveMgr> saveFailed;

private:

			AutoSaveMgr();
			~AutoSaveMgr();

    ObjectSet<AutoSaveable> savers_;
    mutable Threads::Lock lock_;
    Threads::Thread*	thread_;
    bool		appexits_;
    bool		active_;

    void		appExits(CallBacker*);
    void		saverDel(CallBacker*);
    void		survChg( CallBacker* )	    { setEmpty(); }

    void		go();
    static inline void	goCB(CallBacker*);

public:

    // Probably not useful 4 u:

    static AutoSaveMgr&	getInst();		    // use OD::AUTOSAVE()
    void		remove(AutoSaveable*);	    // done 4 u on destruction
    void		setEmpty();		    // why would u?
    Threads::Thread&	thread()		    { return *thread_; }

};


mGlobal(General) inline AutoSaveMgr& AutoSaveMGR()
{ return AutoSaveMgr::getInst(); }


}; //namespace OD

#endif
