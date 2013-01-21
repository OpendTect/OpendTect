#ifndef uiodmain_h
#define uiodmain_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Dec 2003
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiodmainmod.h"
#include "uimainwin.h"
#include "multiid.h"

class CtxtIOObj;
class IOObj;
class IOPar;
class ODSession;
class Timer;
class uicMain;
class uiDockWin;
class uiODApplMgr;
class uiODMain;
class uiODMenuMgr;
class uiODSceneMgr;
class uiODViewer2DMgr;
class uiVisColTabEd;


mGlobal(uiODMain) uiODMain* ODMainWin();
//!< Top-level access for plugins


/*!
\ingroup uiODMain
\brief OpendTect application top level object
*/

mExpClass(uiODMain) uiODMain : public uiMainWin
{
public:

			uiODMain(uicMain&);
			~uiODMain();

    bool		go();
    void		exit();

    uiODApplMgr&	applMgr()	{ return *applmgr_; }
    uiODMenuMgr&	menuMgr()	{ return *menumgr_; } //!< + toolbar
    uiODSceneMgr&	sceneMgr()	{ return *scenemgr_; }
    uiODViewer2DMgr&	viewer2DMgr()	{ return *viewer2dmgr_; }
    uiVisColTabEd&	colTabEd()	{ return *ctabed_; }

    Notifier<uiODMain>	sessionSave;	//!< Put data in pars
    Notifier<uiODMain>	sessionRestoreEarly; //!< Get data from pars, before vis
    Notifier<uiODMain>	sessionRestore;	//!< Get data from pars
    IOPar&		sessionPars();	//!< On session save or restore
    					//!< notification, to get/put data

    Notifier<uiODMain>	justBeforeGo;	//!< Scenes inited, auto-plugins loaded

    bool		askStore(bool& askedanything,const char* actiontype);
    			/*!< Asks user if session, picksets or attributesets
			     need to be stored. */
    bool		askStoreAttribs(bool,bool& askedanything);
    			/*!< Asks user if attributesets (2D or 3D ) 
			  need to be stored. */
    bool		hasSessionChanged(); /*!< Compares current session with 
    						  last saved. */
    void		saveSession();	//!< pops up the save session dialog
    void		restoreSession(); //!< pops up the restore session dlg
    void		autoSession(); //!< pops up the auto session dlg
    bool		isRestoringSession()	{ return restoringsess_; }

protected:

    uiODApplMgr*	applmgr_;
    uiODMenuMgr*	menumgr_;
    uiODSceneMgr*	scenemgr_;
    uiODViewer2DMgr*	viewer2dmgr_;
    uiVisColTabEd*	ctabed_;
    uicMain&		uiapp_;
    ODSession*		cursession_;
    ODSession&		lastsession_;
    bool		restoringsess_;
    uiDockWin*		ctabwin_;

    MultiID		cursessid_;
    bool		failed_;

    virtual bool	closeOK();
    void		afterSurveyChgCB(CallBacker*);
    void		handleStartupSession();
    void		restoreSession(const IOObj*);

private:

    mGlobal(uiODMain) friend int		ODMain(int,char**);

    bool		ensureGoodDataDir();
    bool		ensureGoodSurveySetup();
    bool		buildUI();
    void		initScene();

    CtxtIOObj*		getUserSessionIOData(bool);
    bool		updateSession();
    void		doRestoreSession();

    Timer&		timer_;
    Timer&		memtimer_;
    void		timerCB(CallBacker*);
    void		memTimerCB(CallBacker*);

public:

    bool		sceneMgrAvailable() const	{ return scenemgr_; }
    bool		menuMgrAvailable() const	{ return menumgr_; }
    bool		viewer2DMgrAvailable() const	{ return viewer2dmgr_; }
    void		updateCaption();

};

#endif

