#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uivismod.h"
#include "uidialog.h"

class uiMarkerStyle3D;

mExpClass(uiVis) uiMarkerStyleDlg : public uiDialog
{ mODTextTranslationClass(uiMarkerStyleDlg);
protected:

			uiMarkerStyleDlg(uiParent*,const uiString& title,
					 bool withnone=false);

    uiMarkerStyle3D*	stylefld_;

    bool		acceptOK(CallBacker*);
    virtual void	doFinalize(CallBacker*)		=0;

    virtual void	sliderMove(CallBacker*)		=0;
    virtual void	typeSel(CallBacker*)		=0;
    virtual void	colSel(CallBacker*)		=0;

};
