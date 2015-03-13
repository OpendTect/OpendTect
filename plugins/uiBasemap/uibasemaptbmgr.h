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

class uiBasemapIOMgr;
class uiBasemapView;
class uiMainWin;
class uiSelLineStyle;
class uiToolBar;
class uiToolButton;


mExpClass(uiBasemap) uiBaseMapTBMgr : public CallBacker
{ mODTextTranslationClass(uiBaseMapTBMgr)
public:
			uiBaseMapTBMgr(uiMainWin&,uiBasemapView&);
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
    void		saveAsCB(CallBacker*);
    void		viewCB(CallBacker*);
    void		vworientationCB(CallBacker*);
    void		vwmapscaleCB(CallBacker*);

    void		save(bool saveas);

    bool		isstored_;
    bool		pickmode_;

    int			addid_;
    int			removeid_;

    int			viewid_;
    int			openid_;
    int			saveid_;
    int			saveasid_;
    int			vwmapscaleid_;
    int			vworientationid_;

    uiBasemapIOMgr*	iomgr_;
    uiMainWin&		mainwin_;
    uiBasemapView&	basemapview_;
    uiToolBar*		itemtoolbar_;
    uiToolBar*		vwtoolbar_;
};

#endif
