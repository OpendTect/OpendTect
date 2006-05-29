#ifndef uipickpropdlg_h
#define uipickpropdlg_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2002
 RCS:           $Id: uipickpropdlg.h,v 1.1 2006-05-29 08:02:32 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class uiColorInput;
class uiGenInput;
class uiSliderExtra;

namespace Pick { class Set; };


class uiPickPropDlg : public uiDialog
{
public:
			uiPickPropDlg(uiParent*,Pick::Set&);

protected:

    uiSliderExtra*	sliderfld;
    uiGenInput*		typefld;
    uiColorInput*	colselfld;

    Pick::Set&		set_;

    void		doFinalise(CallBacker*);
    bool		acceptOK(CallBacker*);

    void		sliderMove(CallBacker*);
    void		typeSel(CallBacker*);
    void		colSel(CallBacker*);
};


#endif
