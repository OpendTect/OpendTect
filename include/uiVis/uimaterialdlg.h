#ifndef uimaterialdlg_h
#define uimaterialdlg_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          April 2002
 RCS:           $Id: uimaterialdlg.h,v 1.4 2004-03-02 13:30:02 nanne Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
class uiSlider;
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

    uiSlider*		ambslider;
    uiSlider*		diffslider;
    uiSlider*		specslider;
    uiSlider*		emisslider;
    uiSlider*		shineslider;
    uiSlider*		transslider;

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
