#ifndef uimarkerstyledlg_h
#define uimarkerstyledlg_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May 2002
 RCS:           $Id: uimarkerstyledlg.h,v 1.4 2010-07-21 07:55:31 cvskris Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class uiMarkerStyle3D;

mClass uiMarkerStyleDlg : public uiDialog
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
