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

#include "uiodapplmgr.h"

class DirList;
class uiMenuItem;
class uiODFaultToolMan;
class uiODHelpMenuMgr;
class uiPopupMenu;
class uiToolBar;


/*!\brief The OpendTect menu manager

  The uiODMenuMgr instance can be accessed like:
  ODMainWin()->menuMgr()

  All standard menus should be reachable directly without searching for
  the text. It is easy to add your own menu items. And tool buttons, for that
  matter.

*/

mClass uiODMenuMgr : public CallBacker
{

    friend class	uiODMain;
    friend class	uiODHelpMenuMgr;

public:

    uiPopupMenu*	fileMnu()		{ return surveymnu_; }
    uiPopupMenu*	surveyMnu()		{ return surveymnu_; }
    uiPopupMenu*	analMnu()		{ return analmnu_; }
    uiPopupMenu*	procMnu()		{ return procmnu_; }
    uiPopupMenu*	sceneMnu()		{ return scenemnu_; }
    uiPopupMenu*	viewMnu()		{ return viewmnu_; }
    uiPopupMenu*	utilMnu()		{ return utilmnu_; }
    uiPopupMenu*	helpMnu()		{ return helpmnu_; }
    uiPopupMenu*	settMnu()		{ return settmnu_; }
    uiPopupMenu*	toolsMnu()		{ return toolsmnu_; }
    uiPopupMenu*	preLoadMenu()		{ return preloadmnu_; }

    uiPopupMenu*	getBaseMnu(uiODApplMgr::ActType);
    			//! < Within Survey menu
    uiPopupMenu*	getMnu(bool imp,uiODApplMgr::ObjType);
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

    uiPopupMenu*	surveymnu_;
    uiPopupMenu*	analmnu_;
    uiPopupMenu*	procmnu_;
    uiPopupMenu*	scenemnu_;
    uiPopupMenu*	viewmnu_;
    uiPopupMenu*	utilmnu_;
    uiPopupMenu*	impmnu_;
    uiPopupMenu*	expmnu_;
    uiPopupMenu*	manmnu_;
    uiPopupMenu*	preloadmnu_;
    uiPopupMenu*	helpmnu_;
    uiPopupMenu*	settmnu_;
    uiPopupMenu*	toolsmnu_;
    ObjectSet<uiPopupMenu> impmnus_;
    ObjectSet<uiPopupMenu> expmnus_;

    uiToolBar*		dtecttb_;
    uiToolBar*		cointb_;
    uiToolBar*		mantb_;

    uiODFaultToolMan*	faulttoolman_;

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
    void		manFlt(CallBacker*);
    void		manWll(CallBacker*);
    void		manPick(CallBacker*);
    void		manWvlt(CallBacker*);
    void		manStrat(CallBacker*);
    void		manPDF(CallBacker*);
    void		updateDTectToolBar(CallBacker*);
    void		updateDTectMnus(CallBacker*);
    void		toggViewMode(CallBacker*);
    void		create2D3DMnu(uiPopupMenu*,const char*,int,int,
	    			      const char* pm=0);

    uiMenuItem*		stereooffitm_;
    uiMenuItem*		stereoredcyanitm_;
    uiMenuItem*		stereoquadbufitm_;
    uiMenuItem*		stereooffsetitm_;
    uiMenuItem*		addtimedepthsceneitm_;
    int			axisid_, actviewid_, cameraid_, soloid_;
    int			coltabid_, polyselectid_,viewselectid_,curviewmode_ ;
    int			viewinlid_, viewcrlid_, viewzid_, viewnid_, viewnzid_;
    int			removeselectionid_;
    bool		inviewmode_;

    inline uiODApplMgr&	applMgr()	{ return appl_.applMgr(); }
    inline uiODSceneMgr& sceneMgr()	{ return appl_.sceneMgr(); }

    void		showLogFile();
    void		mkViewIconsMnu();
    void		addIconMnuItems(const DirList&,uiPopupMenu*,
	    				BufferStringSet&);

public:
    uiPopupMenu*       layerModelMnu();
    void 		manProps(CallBacker*);
};


#endif
