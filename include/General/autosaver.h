#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		April 2016
________________________________________________________________________

-*/

#include "generalmod.h"
#include "monitorable.h"
#include "uistring.h"
class IOStream;
class Saveable;


namespace OD
{

class AutoSaver;


mExpClass(General) AutoSaveObj : public CallBacker
{ mODTextTranslationClass(AutoSaveObj)
public:

			AutoSaveObj(const Saveable&,AutoSaver&);
			~AutoSaveObj();

    bool		prevSaveFailed() const	{ return prevsavefailed_; }
    uiRetVal		messages() const	{ return msgs_; }
    bool		isFinished() const;
    const Saveable*	saver() const		{ return saver_; }

private:

    const Saveable*	saver_;
    mutable Threads::Lock lock_;
    AutoSaver&		mgr_;
    mutable uiRetVal	msgs_;
    mutable bool	prevsavefailed_;

    mutable DirtyCountType lastautosavedirtycount_;
    mutable int		lastsaveclockseconds_;
    mutable IOStream*	lastautosaveioobj_;
    mutable int		autosavenr_;

    bool		time4AutoSave() const;
    int			autoSave(bool) const;
    void		removeHiddenSaves();

    void		removeIOObjAndData(IOStream*&) const;
    void		saverDelCB(CallBacker*);

    friend class	AutoSaver;

};


/*!\brief Auto-save manager. Singleton class. Works from its own thread.

  Auto-Save can work in 2 modes: Hidden or !Hidden (visible?). When visible,
  it will simply save as if user explicitly pressed a button or chose a menu
  item to save.

  Hidden mode means it will make a hidden back-up that can be retrieved after an
  emergency (like power failure, server shutdown, maybe even an OpendTect crash
  (which never happens, but still :)) ).

  The default is Hidden.

*/

mExpClass(General) AutoSaver : public CallBacker
{ mODTextTranslationClass(AutoSaver)
public:

    bool		isActive() const;
    void		setActive(bool yn=true);
    bool		useHiddenMode() const;
    void		setUseHiddenMode(bool yn=true);
    int			nrSecondsBetweenSaves() const;
    void		setNrSecondsBetweenSaves(int);

    void		add(const Saveable&);
    bool		restore(IOStream&,const char* newnm);

			// triggered from mgr's thread. CB obj is AutoSaveObj
    Notifier<AutoSaver> saveDone;
    Notifier<AutoSaver> saveFailed;

    static const char*	sKeyAutosaved();

private:

			AutoSaver();
			~AutoSaver();

    mutable Threads::Lock lock_;
    Threads::Thread*	thread_;
    ObjectSet<AutoSaveObj> asobjs_;
    bool		isactive_;
    bool		usehiddenmode_;
    int			curclockseconds_;
    int			nrclocksecondsbetweensaves_;
    Threads::Atomic<bool> appexits_;
    Threads::Atomic<bool> surveychanges_;

    void		remove(AutoSaveObj*);
    void		appExitCB(CallBacker*);
    void		svrDelCB(CallBacker*);
    void		survChgCB(CallBacker*);

    void		go();
    static inline void	goCB(CallBacker*);

    void		handleSurvChg();

    friend class	AutoSaveObj;

public:

    // Probably not useful 4 u:

    static AutoSaver&	getInst();		    // use OD::AUTOSAVE()
    void		setEmpty();		    // try setActive(false)
    Threads::Thread&	thread()		    { return *thread_; }

};


mGlobal(General) inline AutoSaver& AUTOSAVE()
{ return AutoSaver::getInst(); }


}; //namespace OD
