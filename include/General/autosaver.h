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
#include "notify.h"
#include "uistring.h"
class IOStream;


namespace OD
{

class AutoSaveObj;
class Saveable;


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

			// triggered from mgr's thread. CB obj is the saver.
    Notifier<AutoSaver> saveDone;
    Notifier<AutoSaver> saveFailed;

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

#endif
