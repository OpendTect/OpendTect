#ifndef uiodviewer2dposdlg_h
#define uiodviewer2dposdlg_h

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
#include "uiodviewer2dposgrp.h"
#include "uistring.h"


mExpClass(uiODMain) uiODViewer2DPosDlg : public uiDialog
{ mODTextTranslationClass(uiODViewer2DPosDlg);
public:

			uiODViewer2DPosDlg(uiODMain&);

protected:
    uiODViewer2DPosGrp* posgrp_;
    Viewer2DPosDataSel	posdatasel_;
    uiODMain&		odappl_;

    bool		acceptOK(CallBacker*);

};

#endif

