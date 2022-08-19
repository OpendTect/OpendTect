#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
    int			horbut_ = -1;
    int			hormrkdispbut_ = -1;
    uiIOObjSelDlg*	selhordlg_ = nullptr;
    uiWorldRect		curview_;

    uiMrkDispDlg*	mrkrdlg_ = nullptr;
    Server&		server_;

    uiToolBar* toolBar() override { return toolbar_; }
    uiToolBar* editToolBar() override { return toolbar_; }

    bool		checkIfInside(double,double);
    bool		handleUserClick(int vwridx) override;

    void		applyProperties(CallBacker*) override;
    void		viewChangedCB(CallBacker*);
    void		keyPressCB(CallBacker*) override;
    void		loadHorizons(CallBacker*);
    void		dispHorMrks(CallBacker*);
    void		rubBandCB(CallBacker*) override;
    void		reDrawNeeded(CallBacker*);
    void		wheelMoveCB(CallBacker*) override;

    friend class	uiTieWin;
};

} // namespace WellTie
