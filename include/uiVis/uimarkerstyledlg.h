#ifndef uimarkerstyledlg_h
#define uimarkerstyledlg_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May 2002
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uivismod.h"
#include "uidialog.h"

class uiMarkerStyle3D;

mClass(uiVis) uiMarkerStyleDlg : public uiDialog
{
protected:

    			uiMarkerStyleDlg(uiParent*,const char* title);

    uiMarkerStyle3D*	stylefld_;

    bool		acceptOK(CallBacker*);
    virtual void	doFinalise(CallBacker*)		=0;

    virtual void	sliderMove(CallBacker*)		=0;
    virtual void	typeSel(CallBacker*)		=0;
    virtual void	colSel(CallBacker*)		=0;

};

#endif

