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


/*!\brief Temporary screen recorder dialog used to verify UI wiring. */

mExpClass(uiODMain) uiODScreenRecorderDlg : public uiDialog
{ mODTextTranslationClass(uiODScreenRecorderDlg);
public:
	uiODScreenRecorderDlg(uiODMain&);
	~uiODScreenRecorderDlg();
	mOD_DisableCopy(uiODScreenRecorderDlg)
	QString environmentQt(const char* name) const;
};
