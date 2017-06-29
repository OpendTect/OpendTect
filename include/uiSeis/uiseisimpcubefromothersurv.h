#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Oct 2010
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uidialog.h"
#include "seiscubeimpfromothersurv.h"

class uiGenInput;
class uiLabeledSpinBox;
class uiSeisSel;
class uiSeisSubSel;


mExpClass(uiSeis) uiSeisImpCubeFromOtherSurveyDlg : public uiDialog
{ mODTextTranslationClass(uiSeisImpCubeFromOtherSurveyDlg)
public:

			uiSeisImpCubeFromOtherSurveyDlg(uiParent*);
			~uiSeisImpCubeFromOtherSurveyDlg();

protected:

    SeisCubeImpFromOtherSurvey* import_;
    SeisCubeImpFromOtherSurvey::Interpol interpol_;
    bool		issinc_;

    uiGenInput*		finpfld_;
    uiSeisSel*		outfld_;
    uiGenInput*		interpfld_;
    uiLabeledSpinBox*	cellsizefld_;
    uiSeisSubSel*	subselfld_;

    bool		acceptOK();
    void		cubeSel(CallBacker*);
    void		interpSelDone(CallBacker*);
};
