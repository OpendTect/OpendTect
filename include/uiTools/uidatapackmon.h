#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"

#include "uidialog.h"

class Timer;
class uiTextEdit;
class uiTextBrowser;


mExpClass(uiTools) uiDataPackMonitor : public uiDialog
{ mODTextTranslationClass(uiStreamMonitor);
public:

			uiDataPackMonitor(uiParent*,int refreshinsec);
			~uiDataPackMonitor();

    static void		launchFrom(uiParent*,int refreshinsec=3);

private:

    uiTextEdit*		txtbr_;
    Timer&		updatetimer_;
    int			updateinmssec_;
    bool		canupdate_ = true;

    void		initDlg(CallBacker*);
    void		refreshCB(CallBacker*);
    void		deletedCB(CallBacker*);
    bool		acceptOK(CallBacker*) override;

};
