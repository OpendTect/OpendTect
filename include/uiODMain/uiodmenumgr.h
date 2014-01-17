#ifndef uiodmenumgr_h
#define uiodmenumgr_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Dec 2003
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiodmainmod.h"
#include "uiodapplmgr.h"

class DirList;
class MeasureToolMan;
class uiODFaultToolMan;
class uiODHelpMenuMgr;
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
{

    friend class	uiODMain;
    friend class	uiODHelpMenuMgr;

public:

    uiMenu*		fileMnu()		{ return surveymnu_; }
    uiMenu*		surveyMnu()		{ return surveymnu_; }
    uiMenu*		analMnu()		{ return analmnu_; }
    uiMenu*		analWellMnu()		{ return analwellmnu_; }
    uiMenu*		layerModelMnu()		{ return layermodelmnu_; }
    uiMenu*		procMnu()		{ return procmnu_; }
    uiMenu*		sceneMnu()		{ return scenemnu_; }
    uiMenu*		viewMnu()		{ return viewmnu_; }
    uiMenu*		utilMnu()		{ return utilmnu_; }
    uiMenu*		helpMnu()		{ return helpmnu_; }
    uiMenu*		settMnu()		{ return settmnu_; }
    uiMenu*		toolsMnu()		{ return toolsmnu_; }
    uiMenu*		preLoadMenu()		{ return preloadmnu_; }

    uiMenu*		getBaseMnu(uiODApplMgr::ActType);
			//! < Within Survey menu
    uiMenu*		getMnu(bool imp,uiODApplMgr::ObjType);
			//! < Within Survey - Import or Export

    uiToolBar*		dtectTB()		{ return dtecttb_; }
    uiToolBar*		coinTB()		{ return cointb_; }
    uiToolBar*		manTB()			{ return mantb_; }

			// Probably not needed by plugins
    void		updateStereoMenu();
    void		updateViewMode(bool);
    void		updateAxisMode(bool);
    bool		isSoloModeOn() const;
    void		enableMenuBar(bool);
    void		enableActButton(bool);
    void		setCameraPixmap(bool isperspective);
    void		insertNewSceneItem(uiAction*,int id);
    void		updateSceneMenu();

    static int		ask2D3D(const char* txt,int res2d=2,int res3d=3,
				int rescncl=0);

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
    ObjectSet<uiMenu>	impmnus_;
    ObjectSet<uiMenu>	expmnus_;

    uiToolBar*		dtecttb_;
    uiToolBar*		cointb_;
    uiToolBar*		mantb_;

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
    void		create2D3DMnu(uiMenu*,const char*,int,int,
				      const char* pm=0);

    uiAction*		stereooffitm_;
    uiAction*		stereoredcyanitm_;
    uiAction*		stereoquadbufitm_;
    uiAction*		stereooffsetitm_;
    uiAction*		addtimedepthsceneitm_;
    uiAction*		lastsceneitm_;
    int			axisid_, actviewid_, cameraid_, soloid_;
    int			coltabid_, polyselectid_,viewselectid_,curviewmode_ ;
    int			viewinlid_, viewcrlid_, viewzid_, viewnid_, viewnzid_;
    int			removeselectionid_;
    bool		inviewmode_;

    inline uiODApplMgr&	applMgr()	{ return appl_.applMgr(); }
    inline uiODSceneMgr& sceneMgr()	{ return appl_.sceneMgr(); }

    void		showLogFile();
    void		mkViewIconsMnu();
    void		addIconMnuItems(const DirList&,uiMenu*,
					BufferStringSet&);
};


#endif

