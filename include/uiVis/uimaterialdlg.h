#ifndef uimaterialdlg_h
#define uimaterialdlg_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          April 2002
 RCS:           $Id: uimaterialdlg.h,v 1.3 2003-11-07 12:21:54 bert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
class uiLabeledSlider;
namespace visBase { class Material; };


class uiMaterialDlg : public uiDialog
{
public:
			uiMaterialDlg(uiParent*,visBase::Material*,
				      bool ambience=true,
				      bool diffusecolor=true,
				      bool specularcolor=true,
				      bool emmissivecolor=true,
				      bool shininess=true,
				      bool transparency=true );

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
