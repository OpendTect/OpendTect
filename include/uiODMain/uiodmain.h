#ifndef uiodmain_h
#define uiodmain_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Dec 2003
 RCS:           $Id: uiodmain.h,v 1.7 2004-01-26 13:00:35 nanne Exp $
________________________________________________________________________

-*/

#include "uimainwin.h"
class IOPar;
class uicMain;
class uiODMain;
class ODSession;
class CtxtIOObj;
class uiDockWin;
class uiODApplMgr;
class uiODMenuMgr;
class uiODSceneMgr;
class uiVisColTabEd;


uiODMain* ODMainWin();
//!< Top-level access for plugins


/*!\brief OpendTect application top level object */

class uiODMain : public uiMainWin
{
public:

			uiODMain(uicMain&);
			~uiODMain();

    bool		go();
    void		exit();

    uiODApplMgr&	applMgr()	{ return *applmgr; }
    uiODMenuMgr&	menuMgr()	{ return *menumgr; } //!< + toolbar
    uiODSceneMgr&	sceneMgr()	{ return *scenemgr; }
    uiVisColTabEd&	colTabEd()	{ return *ctabed; }

    Notifier<uiODMain>	sessionSave;	//!< When triggered, put data in pars
    Notifier<uiODMain>	sessionRestore;	//!< When triggered, get data from pars
    IOPar&		sessionPars();	//!< On session save or restore
    					//!< notification, to get/put data

    bool		hasSessionChanged(); /*!< Compares current session with 
    						  last saved. */
    void		saveSession();	//!< pops up the save session dialog
    void		restoreSession(); //!< pops up the restore session dlg

protected:

    uiODApplMgr*	applmgr;
    uiODMenuMgr*	menumgr;
    uiODSceneMgr*	scenemgr;
    uiVisColTabEd*	ctabed;
    uicMain&		uiapp;
    ODSession*		cursession;
    ODSession&		lastsession;
    uiDockWin*		ctabwin;

    bool		failed;

    virtual bool	closeOK();

private:

    bool		ensureGoodDataDir();
    bool		ensureGoodSurveySetup();
    bool		buildUI();

    CtxtIOObj*		getUserSessionIOData(bool);
    bool		updateSession();
    void		doRestoreSession();

public:

    bool		sceneMgrAvailable() const	{ return scenemgr; }
    bool		menuMgrAvailable() const	{ return menumgr; }

};


#endif
