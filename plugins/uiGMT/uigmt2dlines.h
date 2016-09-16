#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		August 2008
________________________________________________________________________

-*/

#include "uigmtoverlay.h"

class uiCheckBox;
class uiGenInput;
class uiSeis2DLineSel;
class uiSelLineStyle;
class uiSpinBox;

mClass(uiGMT) uiGMT2DLinesGrp : public uiGMTOverlayGrp
{ mODTextTranslationClass(uiGMT2DLinesGrp);
public:

    			~uiGMT2DLinesGrp();

    static void		initClass();

    bool		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);
    void		reset();

protected:

    			uiGMT2DLinesGrp(uiParent*);

    static uiGMTOverlayGrp*	createInstance(uiParent*);
    static int			factoryid_;

    uiSeis2DLineSel*	lineselfld_;
    uiGenInput*		namefld_;
    uiSelLineStyle*	lsfld_;
    uiCheckBox*		labelfld_;
    uiGenInput*		labelposfld_;
    uiSpinBox*		labelfontfld_;
    uiCheckBox*		trclabelfld_;
    uiGenInput*		trcstepfld_;

    void		labelSel(CallBacker*);
};
