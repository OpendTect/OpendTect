/*+
________________________________________________________________________

    CopyRight:     (C) dGB Beheer B.V.
    Author:        Nageswara
    Date:          0
    RCS:           $Id: uimeasuredlg.h,v 1.3 2008-08-03 18:08:30 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class Coord3;
class LineStyle;
class uiGenInput;
class uiSelLineStyle;

class uiMeasureDlg : public uiDialog
{
public:
				uiMeasureDlg(uiParent*);
				~uiMeasureDlg();

    void			setLineStyle(const LineStyle&);
    const LineStyle&		getLineStyle() const	{ return ls_; }

    void			fill(TypeSet<Coord3>&);
    void			reset();

    Notifier<uiMeasureDlg>	lineStyleChange;
    Notifier<uiMeasureDlg>	clearPressed;

protected:

    LineStyle&			ls_;

    uiGenInput*			hdistfld_;
    uiGenInput*			zdistfld_;
    uiGenInput*			appvelfld_;
    uiGenInput*			distfld_;
    uiGenInput*			inlcrldistfld_;
    uiSelLineStyle*		linestylefld_;

    void			lsChangeCB(CallBacker*);
    void			clearCB(CallBacker*);
    void			stylebutCB(CallBacker*);
};
