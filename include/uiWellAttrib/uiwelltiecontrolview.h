#pragma once

/*+
  ________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          Feb 2009
________________________________________________________________________

-*/

#include "uiwellattribmod.h"
#include "uiflatviewstdcontrol.h"
#include "uistring.h"

class uiFlatViewer;
class uiToolBar;
class uiIOObjSelDlg;
class uiRect;

namespace WellTie
{

class DispParams;
class uiMrkDispDlg;
class Server;

mExpClass(uiWellAttrib) uiControlView : public uiFlatViewStdControl
{ mODTextTranslationClass(uiControlView);
public:
			uiControlView(uiParent*,uiToolBar*,
					uiFlatViewer*,Server&);
			~uiControlView() { detachAllNotifiers(); }

    void		setSelView(bool isnewsel = true, bool viewall=false );

    void		usePar(const IOPar& iop);
    void		fillPar(IOPar& iop) const;

    Notifier<uiControlView> redrawNeeded;
    Notifier<uiControlView> redrawAnnotNeeded;

protected:

    uiToolBar*		toolbar_;
    uiToolButton*	horbut_;
    uiToolButton*	hormrkdispbut_;
    uiIOObjSelDlg*	selhordlg_;
    uiWorldRect		curview_;

    uiMrkDispDlg*	mrkrdlg_;
    Server&		server_;

    bool		checkIfInside(double,double);
    bool		handleUserClick(int vwridx);

    void                applyProperties(CallBacker*);
    void		viewChangedCB(CallBacker*);
    void		keyPressCB(CallBacker*);
    void		loadHorizons(CallBacker*);
    void		dispHorMrks(CallBacker*);
    void		rubBandCB(CallBacker*);
    void		reDrawNeeded(CallBacker*);
    void		wheelMoveCB(CallBacker*);

    friend class	uiTieWin;
};

} // namespace WellTie
