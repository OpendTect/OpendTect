#ifndef uimarkerstyledlg_h
#define uimarkerstyledlg_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2002
 RCS:           $Id: uimarkerstyledlg.h,v 1.2 2009-01-08 10:37:54 cvsranojay Exp $
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
