#ifndef uiodmenumgr_h
#define uiodmenumgr_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Dec 2003
 RCS:           $Id: uiodmenumgr.h,v 1.3 2003-12-25 19:42:23 bert Exp $
________________________________________________________________________

-*/

#include "uiodmain.h"
class Timer;
class uiToolBar;
class uiMenuItem;
class uiPopupMenu;


/*!\brief 3D application top level object - call from main(). */

class uiODMenuMgr
{
public:

			uiODMenuMgr(uiODMain*);

    uiPopupMenu*	fileMnu()		{ return filemnu; }
    uiPopupMenu*	procMnu()		{ return procmnu; }
    uiPopupMenu*	winMnu()		{ return winmnu; }
    uiPopupMenu*	viewMnu()		{ return viewmnu; }
    uiPopupMenu*	utilMnu()		{ return utilmnu; }
    uiPopupMenu*	helpMnu()		{ return helpmnu; }
    uiPopupMenu*	settMnu()		{ return settmnu; }

    uiPopupMenu*	getBaseMnu(uiODMain::ActType);
    			//! < Within File menu
    uiPopupMenu*	getMnu(bool imp,uiODMain::ObjType);
    			//! < Within File - Import or Export

    uiToolBar*		dtectTB()		{ return dtecttb; }
    uiToolBar*		coinTB()		{ return cointb; }


    void		storePositions();
    void		updateStereoMenu(bool stereo,bool quad);
    void		updateViewMode(bool);
    void		updateAxisMode(bool);
    void		enableMenubar(bool);
    void		enableActButton(bool);

protected:

    uiODMain&		appl;
    uiODApplMgr&	applmgr;
    Timer&		timer;

    uiPopupMenu*	filemnu;
    uiPopupMenu*	procmnu;
    uiPopupMenu*	winmnu;
    uiPopupMenu*	viewmnu;
    uiPopupMenu*	utilmnu;
    uiPopupMenu*	impmnu;
    uiPopupMenu*	expmnu;
    uiPopupMenu*	manmnu;
    uiPopupMenu*	helpmnu;
    uiPopupMenu*	settmnu;
    ObjectSet<uiPopupMenu> impmnus;
    ObjectSet<uiPopupMenu> expmnus;

    uiToolBar*		dtecttb;
    uiToolBar*		cointb;

    void		fillFileMenu();
    void		fillProcMenu();
    void		fillWinMenu();
    void		fillViewMenu();
    void		fillUtilMenu();
    void		fillHelpMenu();
    void		fillDtectTB();
    void		fillCoinTB();

    void		handleClick(CallBacker*);
    void		timerCB(CallBacker*);

    uiMenuItem*		stereooffitm;
    uiMenuItem*		stereoredcyanitm;
    uiMenuItem*		stereoquadbufitm;
    uiMenuItem*		stereooffsetitm;
    int			axisid, actid, viewid;

    inline uiODApplMgr&	applMgr()	{ return appl.applMgr(); }
    inline uiODSceneMgr& sceneMgr()	{ return appl.sceneMgr(); }
};


#endif
