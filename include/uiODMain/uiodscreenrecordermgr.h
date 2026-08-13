/** @file include/uiODMain/uiodscreenrecordermgr.h */

#pragma once

/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodmainmod.h"

#include "callback.h"
#include "uistring.h"

class uiODMain;
class uiODScreenRecorderDlg;


/*!\brief Owns the OD screen recorder user interface and recording state. */

mExpClass(uiODMain) uiODScreenRecorderMgr : public CallBacker
{ mODTextTranslationClass(uiODScreenRecorderMgr);
public:
    uiODScreenRecorderMgr(uiODMain&);
	~uiODScreenRecorderMgr();
	mOD_DisableCopy(uiODScreenRecorderMgr)

    void toggleRecording();

protected:

    uiODMain& appl_;
    uiODScreenRecorderDlg* dialog_ {nullptr};
};
