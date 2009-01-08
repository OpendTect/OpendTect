#ifndef uipickpropdlg_h
#define uipickpropdlg_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2002
 RCS:           $Id: uipickpropdlg.h,v 1.6 2009-01-08 10:37:54 cvsranojay Exp $
________________________________________________________________________

-*/

#include "uimarkerstyledlg.h"

class uiCheckBox;

namespace Pick { class Set; };
namespace visSurvey { class PickSetDisplay; };


mClass uiPickPropDlg : public uiMarkerStyleDlg
{
public:
				uiPickPropDlg(uiParent* p,
					      Pick::Set& set, 
					      visSurvey::PickSetDisplay* psd);

protected:

    void			doFinalise(CallBacker*);
    void			sliderMove(CallBacker*);
    void			typeSel(CallBacker*);
    void			colSel(CallBacker*);
    bool			acceptOK(CallBacker*);
    void			drawStyleCB(CallBacker*);
    void			drawSel(CallBacker*);
    
    uiGenInput*         	drawstylefld_;			
    uiCheckBox*         	usedrawstylefld_;			

    Pick::Set&			set_;
    visSurvey::PickSetDisplay*	psd_;
    bool			needtriangulate_;
};

#endif
