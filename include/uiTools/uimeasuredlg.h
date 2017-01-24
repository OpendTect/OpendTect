/*+
________________________________________________________________________

    (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
    Author:	Nanne Hemstra
    Date:	July 2008
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uidialog.h"

namespace OD { class LineStyle; };
class uiConstantVel;
class uiGenInput;
class uiSelLineStyle;
class uiCheckBox;

mExpClass(uiTools) uiMeasureDlg : public uiDialog
{ mODTextTranslationClass(uiMeasureDlg);
public:
				uiMeasureDlg(uiParent*);
				~uiMeasureDlg();

    const OD::LineStyle&		getLineStyle() const	{ return ls_; }

    void			fill(const TypeSet<Coord3>&);
    void			reset();
    bool			doClear() const;

    Notifier<uiMeasureDlg>	lineStyleChange;
    Notifier<uiMeasureDlg>	clearPressed;
    Notifier<uiMeasureDlg>	velocityChange;

protected:

    float			velocity_;
    OD::LineStyle&			ls_;

    uiGenInput*			hdistfld_;
    uiGenInput*			zdistfld_;
    uiGenInput*			zdist2fld_;
    uiConstantVel*		appvelfld_;
    uiGenInput*			distfld_;
    uiGenInput*			dist2fld_;
    uiGenInput*			inlcrldistfld_;
    uiCheckBox*			clearchkbut_;

    void			lsChangeCB(CallBacker*);
    void			clearCB(CallBacker*);
    void			stylebutCB(CallBacker*);
    void			velocityChgd(CallBacker*);
};
