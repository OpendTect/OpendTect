#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Oct 2010
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uidialog.h"
#include "seiscbvsimpfromothersurv.h"

class uiGenInput;
class uiLabeledSpinBox;
class uiSeisSel;
class uiSeisSubSel;


mExpClass(uiSeis) uiSeisImpCBVSFromOtherSurveyDlg : public uiDialog
{ mODTextTranslationClass(uiSeisImpCBVSFromOtherSurveyDlg)
public:

			uiSeisImpCBVSFromOtherSurveyDlg(uiParent*);

protected:

    SeisImpCBVSFromOtherSurvey* import_;
    SeisImpCBVSFromOtherSurvey::Interpol interpol_;
    bool		issinc_;

    uiGenInput*		finpfld_;
    uiSeisSel*		outfld_;
    uiGenInput*		interpfld_;
    uiLabeledSpinBox*	cellsizefld_;
    uiSeisSubSel*	subselfld_;

    bool		acceptOK(CallBacker*);
    void		cubeSel(CallBacker*);
    void		interpSelDone(CallBacker*);
};

