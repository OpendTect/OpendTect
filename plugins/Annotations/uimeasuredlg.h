/*+
________________________________________________________________________

    CopyRight:     (C) dGB Beheer B.V.
    Author:        Nageswara
    Date:          0
    RCS:           $Id: uimeasuredlg.h,v 1.2 2008-08-01 12:12:38 cvsnanne Exp $
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

    const LineStyle&		getLineStyle() const;

    void			fill(TypeSet<Coord3>&);
    void			reset();

    Notifier<uiMeasureDlg>	propertyChange;
    Notifier<uiMeasureDlg>	clearPressed;

protected:

    uiGenInput*			hdistfld_;
    uiGenInput*			zdistfld_;
    uiGenInput*			appvelfld_;
    uiGenInput*			distfld_;
    uiGenInput*			inlcrldistfld_;
    uiSelLineStyle*		linestylefld_;

    void			changeCB(CallBacker*);
    void			clearCB(CallBacker*);
    void			styleCB(CallBacker*);
};
