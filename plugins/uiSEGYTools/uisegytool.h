#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uisegycommon.h"
#include "iopar.h"
#include "uistring.h"
class uiParent;
class uiSEGYRead;


mExpClass(uiSEGYTools) uiSEGYTool : public CallBacker
{ mODTextTranslationClass(uiSEGYTool);

			uiSEGYTool(uiParent*,IOPar* previop=0,int choice=-1);

    void		go();

protected:

    uiParent*		parent_;
    bool		isnext_;
    IOPar		pars_;
    int			choice_;

    uiSEGYRead*		segyread_;

    bool		launchSEGYWiz();
    void		doVSPTool();

    void		toolEnded(CallBacker*);

};
