#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodmainmod.h"
#include "uimainwin.h"
#include "multiid.h"

class CtxtIOObj;
class IOObj;
class ODSession;
class Timer;
class uiMain;
class uiDockWin;
class uiODApplMgr;
class uiODMain;
class uiODMenuMgr;
class uiODSceneMgr;
class uiODViewer2DMgr;
class uiServiceClientMgr;
class uiToolBar;
class uiVisColTabEd;


mGlobal(uiODMain) uiODMain* ODMainWin();
//!< Top-level access for plugins
class uiODRequestServerMgr;

/*!
\brief OpendTect application top level object
*/

mExpClass(uiODMain) uiODMain : public uiMainWin
{mODTextTranslationClass(uiODMain)
public:

			uiODMain(uiMain&);
			~uiODMain();

    void		go();
    void		restart(bool interact=true,bool doconfirm=true);
    void		exit(bool interact=true,bool doconfirm=true,
			     int retcode=0);

    uiODApplMgr&	applMgr()	{ return *applmgr_; }
    uiODMenuMgr&	menuMgr()	{ return *menumgr_; } //!< + toolbar
    uiODSceneMgr&	sceneMgr()	{ return *scenemgr_; }
    uiODViewer2DMgr&	viewer2DMgr()	{ return *viewer2dmgr_; }
    uiVisColTabEd&	colTabEd()	{ return *ctabed_; }
    uiToolBar*		colTabToolBar()	{ return ctabtb_; }
    uiServiceClientMgr& serviceMgr();

    Notifier<uiODMain>	sessionSave;	//!< Put data in pars
    Notifier<uiODMain>	sessionRestoreEarly; //!< Get data from pars, before vis
    Notifier<uiODMain>	sessionRestore;	//!< Get data from pars
    Notifier<uiODMain>	beforeExit;
    IOPar&		sessionPars();	//!< On session save or restore
					//!< notification, to get/put data

    Notifier<uiODMain>	justBeforeGo;	//!< Scenes inited, auto-plugins loaded
    uiString		getProgramName();

    bool		askStore(bool& askedanything,
				 const uiString& actiontype);
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
    uiStringSet&	restoreMsgs()		{ return restoremsgs_; }
    void		setProgramName(const char*);
			//Default is "OpendTect"
    void		setProgInfo(const char*);

    static uiString	sODDesc()
				{ return tr("OpendTect Main Window"); }

protected:

    uiODApplMgr*	applmgr_;
    uiODMenuMgr*	menumgr_;
    uiODSceneMgr*	scenemgr_;
    uiODViewer2DMgr*	viewer2dmgr_;
    uiVisColTabEd*	ctabed_;
    uiToolBar*		ctabtb_;
    uiMain&		uiapp_;
    ODSession*		cursession_	= nullptr;
    ODSession&		lastsession_;
    bool		restoringsess_	= false;
    bool		restarting_	= false;
    BufferString	programname_;
    BufferString	programinfo_;
    uiStringSet		restoremsgs_;

    MultiID		cursessid_;
    bool		failed_		= true;

    bool		closeOK() override	{ return closeOK(true,true); }
    bool		closeOK(bool interact,bool doconfirm);
    bool		prepareRestart(bool interact,bool doconfirm);
    void		afterStartupCB(CallBacker*);
    void		afterSurveyChgCB(CallBacker*);
    void		handleStartupSession();
    void		restoreSession(const IOObj*);

private:

    bool		buildUI();

    CtxtIOObj*		getUserSessionIOData(bool);
    bool		updateSession();
    void		doRestoreSession();

    Timer&		sesstimer_;
    Timer&		memtimer_;
    Timer&		newsurvinittimer_;
    Timer&		autoloadsessiontimer_;

    bool		neednewsurvinit_	= false;
    void		sessTimerCB(CallBacker*);
    void		memTimerCB(CallBacker*);
    void		newSurvInitTimerCB(CallBacker*);
    void		autoloadSessionCB(CallBacker*);

    void		updateCaptionCB(CallBacker*);

    void		checkUpdateAvailable();
    void		updateStatusCB(CallBacker*);
    uiString		getProgramString() const;
    void		translateText() override;

public:

    void		initScene();
    bool		sceneMgrAvailable() const	{ return scenemgr_; }
    bool		menuMgrAvailable() const	{ return menumgr_; }
    bool		viewer2DMgrAvailable() const	{ return viewer2dmgr_; }
    void		updateCaption();

};


mExpClass(uiODMain) uiPluginInitMgr : public CallBacker
{
public:
    virtual		~uiPluginInitMgr();
			mOD_DisableCopy(uiPluginInitMgr)

    uiODMain&		appl()				{ return appl_; }

protected:
			uiPluginInitMgr();
    virtual void	init();

    virtual void	applicationClosing()		{ cleanup(); }
    virtual void	beforeSurveyChange()		{ cleanup(); }
    virtual void	afterSurveyChange()		{}
    virtual void	dTectMenuChanged()		{}
    virtual void	dTectToolbarChanged()		{}
    virtual void	treeAdded(const SceneID&)	{}
    virtual void	cleanup()			{}

    uiODMain&		appl_;

private:
    void		applCloseCB(CallBacker*);
    void		beforeSurvChgCB(CallBacker*);
    void		afterSurvChgCB(CallBacker*);
    void		menuChgCB(CallBacker*);
    void		tbChgCB(CallBacker*);
    void		treeAddCB(CallBacker*);
};
