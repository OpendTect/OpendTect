#ifndef uibasemaptbmgr_h
#define uibasemaptbmgr_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Henrique Mageste
 Date:		November 2014
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uibasemapmod.h"
#include "uibasemapitem.h"

class uiMainWin;
class uiSelLineStyle;
class uiSurveyMap;
class uiToolBar;
class uiToolButton;


mExpClass(uiBasemap) uiBaseMapTBMgr : public CallBacker
{ mODTextTranslationClass(uiBaseMapTBMgr)
public:
		uiBaseMapTBMgr(uiMainWin&,uiSurveyMap&);
		~uiBaseMapTBMgr();
private:
    void		createitemTB();
    void		createviewTB();
    void		updateViewMode();

    void		barSettingsCB(CallBacker*);
    void		iconClickCB(CallBacker*);
    void		readCB(CallBacker*);
    void		removeCB(CallBacker*);
    void		saveCB(CallBacker*);
    void		viewCB(CallBacker*);
    void		vworientationCB(CallBacker*);
    void		vwmapscaleCB(CallBacker*);

    bool		pickmode_;
    int			readbut_;
    int			removebut_;
    int			savebut_;
    int			viewbut_;
    int			vwmapscaleid_;
    int			vworientationid_;

    uiMainWin&		mainwin_;
    uiSurveyMap&	basemapview_;
    uiToolBar*		itemtoolbar_;
    uiToolBar*		vwtoolbar_;

};

#endif
