#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodmainmod.h"
#include "uidialog.h"
#include "uistring.h"

class uiODMain;
class uiODViewer2DPosGrp;

mExpClass(uiODMain) uiODViewer2DPosDlg : public uiDialog
{ mODTextTranslationClass(uiODViewer2DPosDlg);
public:
			uiODViewer2DPosDlg(uiODMain&);
			~uiODViewer2DPosDlg();

protected:
    uiODViewer2DPosGrp* posgrp_;
    uiODMain&		odappl_;

    float		initialx1pospercm_;
    float		initialx2pospercm_;

    void		zoomLevelCB(CallBacker*);
    bool		acceptOK(CallBacker*);

};
