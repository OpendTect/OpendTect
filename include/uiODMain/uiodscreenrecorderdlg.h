/** @file include/uiODMain/uiodscreenrecorderdlg.h */

#pragma once

/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodmainmod.h"

#include "uidialog.h"

class uiODMain;
class uiTextEdit;
class QPushButton;


/*!\brief Temporary screen recorder diagnostics dialog. */

mExpClass(uiODMain) uiODScreenRecorderDlg : public uiDialog
{ mODTextTranslationClass(uiODScreenRecorderDlg);
public:
	uiODScreenRecorderDlg(uiODMain&);
	~uiODScreenRecorderDlg();
	mOD_DisableCopy(uiODScreenRecorderDlg)

protected:

    void		refreshCB(CallBacker*);
    void		copyCB(CallBacker*);

    uiTextEdit* diagnosticstxt_	{nullptr};
	uiPushButton* startStopButton_ {nullptr};
};
