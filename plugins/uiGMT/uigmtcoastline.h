#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uigmtoverlay.h"

class uiColorInput;
class uiComboBox;
class uiGenInput;
class uiSelLineStyle;
class uiSpinBox;

mClass(uiGMT) uiGMTCoastlineGrp : public uiGMTOverlayGrp
{ mODTextTranslationClass(uiGMTCoastlineGrp);
public:

    static void		initClass();

    bool		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);
    void		reset();

protected:

    			uiGMTCoastlineGrp(uiParent*);

    static uiGMTOverlayGrp*	createInstance(uiParent*);
    static int			factoryid_;

    uiSpinBox*		utmfld_;
    uiSpinBox*		cmfld_;
    uiGenInput*		ewfld_;
    uiComboBox*		resolutionfld_;
    uiSelLineStyle*	lsfld_;
    uiColorInput*	wetcolfld_;
    uiColorInput*	drycolfld_;

    void		utmSel(CallBacker*);
};
