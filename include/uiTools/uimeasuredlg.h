#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uidialog.h"

namespace OD { class LineStyle; }
class uiCheckBox;
class uiComboBox;
class uiGenInput;
class uiSelLineStyle;

mExpClass(uiTools) uiMeasureDlg : public uiDialog
{ mODTextTranslationClass(uiMeasureDlg);
public:
				uiMeasureDlg(uiParent*);
				~uiMeasureDlg();

    const OD::LineStyle&	getLineStyle() const	{ return ls_; }
    bool			doClear() const;

    void			fill(const TypeSet<Coord3>&);
    void			reset();

    Notifier<uiMeasureDlg>	lineStyleChange;
    Notifier<uiMeasureDlg>	clearPressed;
    Notifier<uiMeasureDlg>	velocityChange;
    Notifier<uiMeasureDlg>	dipUnitChange;

protected:

    float			velocity_;
    OD::LineStyle&		ls_;

    uiGenInput*			hdistfld_;
    uiGenInput*			zdistfld_;
    uiGenInput*			zdist2fld_;
    uiGenInput*			appvelfld_;
    uiGenInput*			distfld_;
    uiGenInput*			dist2fld_;
    uiGenInput*			inlcrldistfld_;
    uiGenInput*			dipfld_;
    uiComboBox*			dipunitfld_;
    uiCheckBox*			clearchkbut_;

    void			finalizeCB(CallBacker*);
    void			lsChangeCB(CallBacker*);
    void			clearCB(CallBacker*);
    void			stylebutCB(CallBacker*);
    void			velocityChgd(CallBacker*);
    void			dipUnitSel( CallBacker* );
};
