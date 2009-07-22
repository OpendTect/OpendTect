#ifndef uimarkerstyledlg_h
#define uimarkerstyledlg_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May 2002
 RCS:           $Id: uimarkerstyledlg.h,v 1.3 2009-07-22 16:01:23 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class uiColorInput;
class uiGenInput;
class uiSliderExtra;


mClass uiMarkerStyleDlg : public uiDialog
{
protected:

    			uiMarkerStyleDlg(uiParent*,const char* title);

    uiSliderExtra*	sliderfld;
    uiGenInput*		typefld;
    uiColorInput*	colselfld;

    bool		acceptOK(CallBacker*);
    virtual void	doFinalise(CallBacker*)		=0;

    virtual void	sliderMove(CallBacker*)		=0;
    virtual void	typeSel(CallBacker*)		=0;
    virtual void	colSel(CallBacker*)		=0;

};

#endif
