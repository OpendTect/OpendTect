/*+
________________________________________________________________________

    (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
    Author:        Nageswara
    Date:          July 2008
    RCS:           $Id$
________________________________________________________________________

-*/

#include "uidialog.h"

class Coord3;
class LineStyle;
class uiGenInput;
class uiSelLineStyle;

mClass(Annotations) uiMeasureDlg : public uiDialog
{
public:
				uiMeasureDlg(uiParent*);
				~uiMeasureDlg();

    const LineStyle&		getLineStyle() const	{ return ls_; }

    void			fill(const TypeSet<Coord3>&);
    void			reset();

    Notifier<uiMeasureDlg>	lineStyleChange;
    Notifier<uiMeasureDlg>	clearPressed;
    Notifier<uiMeasureDlg>	velocityChange;

protected:

    float			velocity_;
    LineStyle&			ls_;

    uiGenInput*			hdistfld_;
    uiGenInput*			zdistfld_;
    uiGenInput*			zdist2fld_;
    uiGenInput*			appvelfld_;
    uiGenInput*			distfld_;
    uiGenInput*			dist2fld_;
    uiGenInput*			inlcrldistfld_;

    void			lsChangeCB(CallBacker*);
    void			clearCB(CallBacker*);
    void			stylebutCB(CallBacker*);
    void			velocityChgd(CallBacker*);
};
