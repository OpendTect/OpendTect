#pragma once

/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Satyaki
Date:	       March 2015
RCS:	       $Id: Exp
$
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

protected:
    uiODViewer2DPosGrp* posgrp_;
    uiODMain&		odappl_;

    float		initialx1pospercm_;
    float		initialx2pospercm_;

    void		zoomLevelCB(CallBacker*);
    bool		acceptOK();

};
