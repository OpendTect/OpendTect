#ifndef uigridlinesdlg_h
#define uigridlinesdlg_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Payraudeau
 Date:          February 2006
 RCS:           $Id: uigridlinesdlg.h,v 1.1 2006-02-09 07:48:06 cvshelene Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class uiGenInput;
class uiCheckBox;
class uiSelLineStyle;

namespace visSurvey { class PlaneDataDisplay; }

class uiGridLinesDlg : public uiDialog
{
public:
			uiGridLinesDlg(uiParent*, visSurvey::PlaneDataDisplay*);

protected:

    void		setParameters();
    void 		showLSSel(CallBacker*);
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
