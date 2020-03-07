#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
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

public:

    uiMenu*	fileMnu()		{ return surveymnu_; }
    uiMenu*	surveyMnu()		{ return surveymnu_; }
    uiMenu*	analMnu()		{ return analmnu_; }
    uiMenu*	analWellMnu()		{ return analwellmnu_; }
    uiMenu*	layerModelMnu()		{ return layermodelmnu_; }
    uiMenu*	procMnu()		{ return procmnu_; }
    uiMenu*	procWellMnu()		{ return procwellmnu_; }
    uiMenu*	sceneMnu()		{ return scenemnu_; }
    uiMenu*	viewMnu()		{ return viewmnu_; }
    uiMenu*	utilMnu()		{ return utilmnu_; }
    uiMenu*	helpMnu()		{ return helpmnu_; }
    uiMenu*	docMnu();
    uiMenu*	settMnu()		{ return settmnu_; }
    uiMenu*	toolsMnu()		{ return toolsmnu_; }
    uiMenu*	installMnu()		{ return installmnu_; }
    uiMenu*	preLoadMenu()		{ return preloadmnu_; }
    uiMenu*	createSeisOutputMenu()	{ return csomnu_; }
    uiMenu*	createHorOutputMenu()	{ return chomnu_; }
    uiMenu*	impWellTrackMenu()	{ return imptrackmnu_; }
    uiMenu*	impWellLogsMenu()	{ return implogsmnu_; }
    uiMenu*	impWellMarkersMenu()	{ return impmarkersmnu_; }
    uiMenu*	mmProcMenu()		{ return mmmnu_; }
    void	createFaultToolMan();

    typedef uiODApplMgr::ActType    ActType;
    typedef uiODApplMgr::ObjType    ObjType;
    uiMenu*	getBaseMnu(ActType); //! < Within Survey menu
    uiMenu*	getMnu(bool imp,ObjType);
			//! < Within Survey - Import or Export

    uiToolBar*	dtectTB()		{ return dtecttb_; }
    uiToolBar*	viewTB()		{ return viewtb_; }
    uiToolBar*	manTB()			{ return mantb_; }
    uiToolBar*	pluginTB();
    uiToolBar*	customTB(const char*);

			// Probably not needed by plugins
    void	updateStereoMenu();
    void	updateViewMode(bool);
    void	updateAxisMode(bool);
    bool	isSoloModeOn() const;
    void	enableMenuBar(bool);
    void	enableActButton(bool);
    void	setCameraPixmap(bool isperspective);
    void	insertNewSceneItem(uiAction*,int id);
    void	updateSceneMenu();

    static int	ask2D3D(const uiString& txt,int res2d=2,int res3d=3,
			int rescncl=0);
    int		add2D3DToolButton(uiToolBar&,const char* iconnnm,
			const uiString& tooltip,
			const CallBack& cb2d,const CallBack& cb3d,
			int itmid2d=-1,int itmid3d=-1);

    uiMenu*	addSubMenu(uiMenu*,const uiString&,const char* icnm);
    uiAction*	addAction(uiMenu*,const uiString&,const char* icnm,
			const CallBack&,int mnuitmid=-1);
    uiAction*	addDirectAction(uiMenu*,const uiString&,const char*,int);
    uiMenu*	add2D3DActions(uiMenu*,const uiString&,const char*,
			const CallBack&,const CallBack&,bool always3d=false,
			int mnuitmid2d=-1,int mnuitmid3d=-1);
    uiMenu*	addFullSeisSubMenu(uiMenu*,const uiString& submnunm,
			const char* icnm,const CallBack&,int menustartid);
		//!< menustartid+0 == 2D, +1 == PS2D, +2 == 3D, +3 == PS3D

    Notifier<uiODMenuMgr> dTectTBChanged;
    Notifier<uiODMenuMgr> dTectMnuChanged;

protected:

			uiODMenuMgr(uiODMain*);
			~uiODMenuMgr();
    void		initSceneMgrDepObjs(uiODApplMgr*,uiODSceneMgr*);

    uiODMain&		appl_;
    uiODHelpMenuMgr*	helpmnumgr_;
    uiODLangMenuMgr*	langmnumgr_;

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
    uiMenu*		toolsmnu_;
    uiMenu*		installmnu_;
    uiMenu*		mmmnu_;
    uiMenu*		csomnu_;
    uiMenu*		chomnu_;
    uiMenu*		imptrackmnu_;
    uiMenu*		implogsmnu_;
    uiMenu*		impmarkersmnu_;
    ObjectSet<uiMenu>	impmnus_;
    ObjectSet<uiMenu>	expmnus_;
    BufferStringSet	iconsetnames_;

    uiToolBar*		dtecttb_;
    uiToolBar*		viewtb_;
    uiToolBar*		mantb_;
    uiToolBar*		plugintb_;
    ObjectSet<uiToolBar> customtbs_;

    uiODFaultToolMan*	faulttoolman_;
    MeasureToolMan*	measuretoolman_;

    uiAction*	addAction(uiMenu*,const uiString&,const char* icnm,int);
    uiAction*	addCheckableAction(uiMenu*,const uiString&,const char*,int);
    uiMenu*	addAsciiSubMenu(uiMenu*,const uiString&,const char*);
    uiMenu*	addAsciiActionSubMenu(uiMenu*,const uiString&,
				      const char* icnm,int,
				      const uiString* altascnm=0);
    uiMenu*	add2D3DActions(uiMenu*,const uiString&,const char*,int,int,
			       bool always3d=false);
    uiMenu*	add2D3DAsciiSubMenu(uiMenu*,const uiString&,
				      const char* icnm,int,int,
				      const uiString* altascnm=0,
				      bool always3d=false);
    uiMenu*	addSingMultAsciiSubMenu(uiMenu*,const uiString&,
				      const char* icnm,int,int,
				      const uiString* altascnm=0);
    uiMenu*	add2D3DSingMultAsciiSubMenu(uiMenu*,
			    const uiString&,const char*,
			    int,int,int,int,bool always3d=false);

    void	fillSurveyMenu();
    void	setSurveySubMenus();
    void	fillAnalMenu();
    void	fillProcMenu();
    void	fillSceneMenu();
    void	fillViewMenu();
    void	fillUtilMenu();
    void	fillDtectTB(uiODApplMgr*);
    void	fillVisTB(uiODSceneMgr*);
    void	fillManTB();
    void	fillWellImpSubMenu(uiMenu*);
    void	fillFullSeisSubMenu(uiMenu*,const CallBack&,int);
    void	createSeisSubMenus();

    void	polySelectionModeCB(CallBacker*);
    void	removeSelectionCB(CallBacker*);
    void	handleToolClick(CallBacker*);
    void	handleViewClick(CallBacker*);
    void	handleClick(CallBacker*);
    void	dispColorBar(CallBacker*);
    void	manSeis(CallBacker*);
    void	manHor(CallBacker*);
    void	manBody(CallBacker*);
    void	manProps(CallBacker*);
    void	manFlt(CallBacker*);
    void	manWll(CallBacker*);
    void	manPick(CallBacker*);
    void	manWvlt(CallBacker*);
    void	manStrat(CallBacker*);
    void	manPDF(CallBacker*);
    void	updateDTectToolBar(CallBacker*);
    void	updateDTectMnus(CallBacker*);
    void	toggViewMode(CallBacker*);

    uiAction*	stereooffitm_;
    uiAction*	stereoredcyanitm_;
    uiAction*	stereoquadbufitm_;
    uiAction*	stereooffsetitm_;
    uiAction*	addtimedepthsceneitm_;
    uiAction*	lastsceneitm_;
    int		axisid_, actviewid_, cameraid_, soloid_;
    int		coltabid_, polyselectid_,viewselectid_,curviewmode_ ;
    int		viewinlid_, viewcrlid_, viewzid_, viewnid_, viewnzid_;
    int		removeselectionid_;
    bool	inviewmode_;

    inline uiODApplMgr&	applMgr()	{ return appl_.applMgr(); }
    inline uiODSceneMgr& sceneMgr()	{ return appl_.sceneMgr(); }

    void	showLogFile();
    void	showFirewallProcDlg();
    void	showHostID();

    uiMenu*	addDualAsciiSubMenu(uiMenu*,const uiString&,
				      const char* icnm,int,int,bool,
				      const uiString*,bool);

    friend class	uiODMain;
    friend class	uiODHelpMenuMgr;
    friend class	uiODLangMenuMgr;

};
