#ifndef uimaterialdlg_h
#define uimaterialdlg_h
/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        N. Hemstra
 Date:          April 2002
 RCS:           $Id: uimaterialdlg.h,v 1.2 2002-04-12 07:10:46 kristofer Exp $
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
