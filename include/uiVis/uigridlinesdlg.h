#ifndef uigridlinesdlg_h
#define uigridlinesdlg_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Payraudeau
 Date:          February 2006
 RCS:           $Id: uigridlinesdlg.h,v 1.4 2009-01-08 10:37:54 cvsranojay Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "ranges.h"

class uiCheckBox;
class uiGenInput;
class uiSelLineStyle;

namespace visSurvey { class PlaneDataDisplay; }

mClass uiGridLinesDlg : public uiDialog
{
public:
			uiGridLinesDlg(uiParent*,visSurvey::PlaneDataDisplay*);

protected:

    void		setParameters();
    void 		showGridLineCB(CallBacker*);
    bool                acceptOK(CallBacker*);

    uiCheckBox*		inlfld_;
    uiCheckBox*		crlfld_;
    uiCheckBox*		zfld_;
    uiGenInput*		inlspacingfld_;
    uiGenInput*		crlspacingfld_;
    uiGenInput*		zspacingfld_;
    uiSelLineStyle*     lsfld_;

    visSurvey::PlaneDataDisplay*	pdd_;
};

#endif
