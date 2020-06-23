/*+
________________________________________________________________________

    (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
    Author:	Nanne Hemstra
    Date:	July 2008
    RCS:	$Id$
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uidialog.h"

namespace OD { class LineStyle; }
class uiGenInput;
class uiSelLineStyle;
class uiCheckBox;
class uiComboBox;

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

    void			lsChangeCB(CallBacker*);
    void			clearCB(CallBacker*);
    void			stylebutCB(CallBacker*);
    void			velocityChgd(CallBacker*);
    void			dipUnitSel( CallBacker* );
};
