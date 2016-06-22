#ifndef uivismarkerstyledlg_h
#define uivismarkerstyledlg_h
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

mExpClass(uiVis) uiVisMarkerStyleDlg : public uiDialog
{ mODTextTranslationClass(uiVisMarkerStyleDlg);
protected:

			uiVisMarkerStyleDlg(uiParent*,const uiString& title);

    uiMarkerStyle3D*	stylefld_;

    bool		acceptOK(CallBacker*);
    virtual void	doFinalise(CallBacker*)		=0;

    virtual void	sizeChg(CallBacker*)		=0;
    virtual void	typeSel(CallBacker*)		=0;
    virtual void	colSel(CallBacker*)		=0;

};

#endif
