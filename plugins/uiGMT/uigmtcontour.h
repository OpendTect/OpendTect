#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		July 2008
________________________________________________________________________

-*/

#include "uigmtoverlay.h"

class uiCheckBox;
class uiColSeqSel;
class uiColSeqUseModeSel;
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

    const EM::Horizon3D* hor_;
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
    uiColSeqSel*	colseqfld_;
    uiColSeqUseModeSel*	sequsefld_;

    void		readCB(CallBacker*);
    void		resetCB(CallBacker*);
    void		rgChg(CallBacker*);
    void		selChg(CallBacker*);
    void		objSel(CallBacker*);
    void		drawSel(CallBacker*);

    bool		loadHor();
};
