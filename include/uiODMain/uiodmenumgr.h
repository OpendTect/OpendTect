#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Dec 2003
________________________________________________________________________

-*/

#include "uiodmainmod.h"
#include "uiodapplmgr.h"

class DirList;
class MeasureToolMan;
class uiODFaultToolMan;
class uiODHelpMenuMgr;
class uiODLangMenuMgr;
class uiMenu;
class uiToolBar;
class uiAction;


/*!\brief The OpendTect menu manager

  The uiODMenuMgr instance can be accessed like:
  ODMainWin()->menuMgr()

  All standard menus should be reachable directly without searching for
  the text. It is easy to add your own menu items. And tool buttons, for that
  matter.

*/

mExpClass(uiODMain) uiODMenuMgr : public CallBacker
{ mODTextTranslationClass(uiODMenuMgr);

    friend class	uiODMain;
    friend class	uiODHelpMenuMgr;

public:

    uiMenu*		fileMnu()		{ return surveymnu_; }
    uiMenu*		surveyMnu()		{ return surveymnu_; }
    uiMenu*		analMnu()		{ return analmnu_; }
    uiMenu*		analWellMnu()		{ return analwellmnu_; }
    uiMenu*		layerModelMnu()		{ return layermodelmnu_; }
    uiMenu*		procMnu()		{ return procmnu_; }
    uiMenu*		procWellMnu()		{ return procwellmnu_; }
    uiMenu*		sceneMnu()		{ return scenemnu_; }
    uiMenu*		viewMnu()		{ return viewmnu_; }
    uiMenu*		utilMnu()		{ return utilmnu_; }
    uiMenu*		helpMnu()		{ return helpmnu_; }
    uiMenu*		docMnu();
    uiMenu*		settMnu()		{ return settmnu_; }
    uiMenu*		toolsMnu()		{ return toolsmnu_; }
    uiMenu*		installMnu()		{ return installmnu_; }
    uiMenu*		preLoadMenu()		{ return preloadmnu_; }
    uiMenu*		createSeisOutputMenu()	{ return csoitm_; }

    uiMenu*		getBaseMnu(uiODApplMgr::ActType);
			//! < Within Survey menu
    uiMenu*		getMnu(bool imp,uiODApplMgr::ObjType);
			//! < Within Survey - Import or Export

    uiToolBar*		dtectTB()		{ return dtecttb_; }
    uiToolBar*		viewTB()		{ return viewtb_; }
    uiToolBar*		manTB()			{ return mantb_; }
    int			manActionID(uiODApplMgr::ObjType) const;
			//! To get access to button in Manage ToolBar
    uiToolBar*		pluginTB();
    uiToolBar*		customTB(const char*);

			// Probably not needed by plugins
    void		updateStereoMenu();
    void		updateViewMode(bool);
    void		updateAxisMode(bool);
    bool		isSoloModeOn() const;
    void		enableMenuBar(bool);
    void		enableActButton(bool);
    void		setCameraPixmap(bool isperspective);
    void		insertNewSceneItem(uiAction*,int id);
    void		insertNewSceneItem(uiMenu*);
    void		updateSceneMenu();

    static int		ask2D3D(const uiString& txt,int res2d=2,int res3d=3,
				int rescncl=0);
    int			add2D3DToolButton(uiToolBar&,const char* iconnnm,
				     const uiString& tooltip,
				     const CallBack& cb2d,const CallBack& cb3d,
				     int itmid2d=-1,int itmid3d=-1);
    void		add2D3DMenuItem(uiMenu&,const char* iconnnm,
				     const uiString& menuitmtxt,
				     const CallBack& cb2d,const CallBack& cb3d,
				     int itmid2d=-1,int itmid3d=-1);

    Notifier<uiODMenuMgr> dTectTBChanged;
    Notifier<uiODMenuMgr> dTectMnuChanged;

protected:

			uiODMenuMgr(uiODMain*);
			~uiODMenuMgr();
    void		initSceneMgrDepObjs(uiODApplMgr*,uiODSceneMgr*);

    uiODMain&		appl_;
    uiODHelpMenuMgr*	helpmgr_;

    uiMenu*		surveymnu_;
    uiMenu*		analmnu_;
    uiMenu*		analwellmnu_;
    uiMenu*		layermodelmnu_;
    uiMenu*		procmnu_;
    uiMenu*		procwellmnu_;
    uiMenu*		scenemnu_;
    uiMenu*		viewmnu_;
    uiMenu*		utilmnu_;
    uiMenu*		impmnu_;
    uiMenu*		expmnu_;
    uiMenu*		manmnu_;
    uiMenu*		preloadmnu_;
    uiMenu*		helpmnu_;
    uiMenu*		settmnu_;
    uiMenu*		langmnu_;
    uiMenu*		toolsmnu_;
    uiMenu*		installmnu_;
    uiMenu*		csoitm_;
    uiODLangMenuMgr* langmnumgr_ = nullptr;
    ObjectSet<uiMenu>	impmnus_;
    ObjectSet<uiMenu>	expmnus_;

    uiToolBar*		dtecttb_;
    uiToolBar*		viewtb_;
    uiToolBar*		mantb_;
    uiToolBar*		plugintb_;
    ObjectSet<uiToolBar>	customtbs_;

    uiODFaultToolMan*	faulttoolman_;
    MeasureToolMan*	measuretoolman_;

    void		fillSurveyMenu();
    void		fillImportMenu();
    void		fillExportMenu();
    void		fillManMenu();
    void		fillAnalMenu();
    void		fillProcMenu();
    void		fillSceneMenu();
    void		fillViewMenu();
    void		fillUtilMenu();
    void		fillDtectTB(uiODApplMgr*);
    void		fillCoinTB(uiODSceneMgr*);
    void		fillManTB();

    void		selectionMode(CallBacker*);
    void		handleToolClick(CallBacker*);
    void		handleViewClick(CallBacker*);
    void		handleClick(CallBacker*);
    void		removeSelection(CallBacker*);
    void		dispColorBar(CallBacker*);
    void		manSeis(CallBacker*);
    void		manHor(CallBacker*);
    void		manBody(CallBacker*);
    void		manProps(CallBacker*);
    void		manFlt(CallBacker*);
    void		manWll(CallBacker*);
    void		manPick(CallBacker*);
    void		manWvlt(CallBacker*);
    void		manStrat(CallBacker*);
    void		manPDF(CallBacker*);
    void		updateDTectToolBar(CallBacker*);
    void		updateDTectMnus(CallBacker*);
    void		toggViewMode(CallBacker*);
    void		toggleTreeMode(CallBacker*);
    void		add2D3DMenuItem(uiMenu&,const char* iconnnm,
					const uiString& menuitmtxt,
					int itmid2d=-1,int itmid3d=-1);
    void		insertAction(uiMenu*,const uiString& txt,int id,
				     const char* iconnm=nullptr);

    uiAction*		stereooffitm_;
    uiAction*		stereoredcyanitm_;
    uiAction*		stereoquadbufitm_;
    uiAction*		stereooffsetitm_;
    uiAction*		addtimedepthsceneitm_;
    uiAction*		lastsceneitm_;
    uiAction*		showtreeitm_			= nullptr;
    int			axisid_, actviewid_, cameraid_, soloid_;
    int			coltabid_, polyselectid_,viewselectid_,curviewmode_ ;
    int			viewinlid_, viewcrlid_, viewzid_, viewnid_, viewnzid_;
    int			removeselectionid_;
    bool		inviewmode_;

    inline uiODApplMgr&	applMgr()	{ return appl_.applMgr(); }
    inline uiODSceneMgr& sceneMgr()	{ return appl_.sceneMgr(); }

    void		showLogFile();
    void		showFirewallProcDlg();
    void		mkViewIconsMnu();
    void		addIconMnuItems(const DirList&,uiMenu*,
					BufferStringSet&);
    void		showHostID();

public:
    void		createFaultToolMan();

private:
			struct ManageActionIDSet
			{
			    int seisid_=-1, horid_=-1, fltid_=-1, wllid_=-1,
				pickid_=-1, bodyid_=-1, wvltid_=-1, stratid_=-1;
			};

    ManageActionIDSet	manids_;
};


