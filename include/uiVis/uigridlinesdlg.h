#ifndef uigridlinesdlg_h
#define uigridlinesdlg_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        H. Payraudeau
 Date:          February 2006
 RCS:           $Id: uigridlinesdlg.h,v 1.6 2012-08-03 13:01:18 cvskris Exp $
________________________________________________________________________

-*/

#include "uivismod.h"
#include "uidialog.h"
#include "ranges.h"

class uiCheckBox;
class uiGenInput;
class uiSelLineStyle;

namespace visSurvey { class PlaneDataDisplay; }

mClass(uiVis) uiGridLinesDlg : public uiDialog
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

