#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uigmtoverlay.h"

class uiCheckBox;
class uiColorTableSel;
class uiComboBox;
class uiGenInput;
class uiIOObjSel;
class uiPosSubSel;
class uiPushButton;
class uiSelLineStyle;

namespace EM { class SurfaceIOData; class Horizon3D; }

mClass(uiGMT) uiGMTContourGrp : public uiGMTOverlayGrp
{ mODTextTranslationClass(uiGMTContourGrp);
public:

    static void		initClass();

    			~uiGMTContourGrp();

    bool		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);
    void		reset();

protected:

    			uiGMTContourGrp(uiParent*);

    static uiGMTOverlayGrp*	createInstance(uiParent*);
    static int			factoryid_;

    EM::Horizon3D*	hor_;
    EM::SurfaceIOData&	sd_;
    Interval<float>	valrg_;

    uiIOObjSel*		inpfld_;
    uiPosSubSel*	subselfld_;
    uiComboBox*		attribfld_;
    uiGenInput*		rgfld_;
    uiPushButton*	resetbut_;
    uiGenInput*		nrcontourfld_;
    uiCheckBox*		linefld_;
    uiSelLineStyle*	lsfld_;
    uiCheckBox*		fillfld_;
    uiColorTableSel*	colseqfld_;
    uiCheckBox*		flipfld_;

    void		readCB(CallBacker*);
    void		resetCB(CallBacker*);
    void		rgChg(CallBacker*);
    void		selChg(CallBacker*);
    void		objSel(CallBacker*);
    void		drawSel(CallBacker*);

    bool		loadHor();
};
