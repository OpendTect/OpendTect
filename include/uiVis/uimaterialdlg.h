#ifndef uimaterialdlg_h
#define uimaterialdlg_h
/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        N. Hemstra
 Date:          April 2002
 RCS:           $Id: uimaterialdlg.h,v 1.1 2002-04-04 16:07:29 nanne Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
class uiLabeledSlider;
namespace visBase { class Material; };


class uiMaterialDlg : public uiDialog
{
public:
			uiMaterialDlg(uiParent*,visBase::Material*);

protected:

    visBase::Material*	material;

    uiLabeledSlider*	ambslider;
    uiLabeledSlider*	diffslider;
    uiLabeledSlider*	specslider;
    uiLabeledSlider*	emisslider;
    uiLabeledSlider*	shineslider;
    uiLabeledSlider*	transslider;

    void		doFinalise(CallBacker*);
    bool		acceptOK(CallBacker*);
    void		ambSliderMove(CallBacker*);
    void		diffSliderMove(CallBacker*);
    void		specSliderMove(CallBacker*);
    void		emisSliderMove(CallBacker*);
    void		shineSliderMove(CallBacker*);
    void		transSliderMove(CallBacker*);

};

#endif
