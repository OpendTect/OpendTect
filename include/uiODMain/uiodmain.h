#ifndef uiodmain_h
#define uiodmain_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Dec 2003
 RCS:           $Id: uiodmain.h,v 1.1 2003-12-20 13:24:05 bert Exp $
________________________________________________________________________

-*/

#include "uimainwin.h"
class uicMain;
class uiODMain;
class uiODApplMgr;
class uiODMenuMgr;
class uiODSceneMgr;


uiODMain* ODMainWin();
//!< Top-level access for plugins


/*!\brief OpendTect application top level object */

class uiODMain : public uiMainWin
{
public:

			uiODMain(int,char**);
			~uiODMain();

    bool		go();
    void		exit();

    uiODApplMgr&	applMgr()	{ return *applmgr; }
    uiODMenuMgr&	menuMgr()	{ return *menumgr; } //!< + toolbar
    uiODSceneMgr&	sceneMgr()	{ return *scenemgr; }

    enum ObjType	{ Seis, Hor, Wll, Attr };
    enum ActType	{ Imp, Exp, Man };

protected:

    uiODApplMgr*	applmgr;
    uiODMenuMgr*	menumgr;
    uiODSceneMgr*	scenemgr;
    uicMain&		uiapp;

    bool		failed;

private:

    bool		ensureGoodDataDir();
    bool		ensureGoodSurveySetup();
    bool		buildUI();
    void		initCT();

};


#endif
