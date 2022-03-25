#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May 2002
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

