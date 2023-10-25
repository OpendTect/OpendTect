#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uidialog.h"
#include "seiscbvsimpfromothersurv.h"
#include "surveydisklocation.h"

class uiGenInput;
class uiLabeledSpinBox;
class uiSeisSel;
class uiSeisSubSel;


mExpClass(uiSeis) uiSeisImpCBVSFromOtherSurveyDlg : public uiDialog
{ mODTextTranslationClass(uiSeisImpCBVSFromOtherSurveyDlg)
public:
			uiSeisImpCBVSFromOtherSurveyDlg(uiParent*);
			~uiSeisImpCBVSFromOtherSurveyDlg();

protected:

    SeisImpCBVSFromOtherSurvey* import_;
    SeisImpCBVSFromOtherSurvey::Interpol interpol_;
    bool		issinc_;
    SurveyDiskLocation	sdl_;

    uiGenInput*		finpfld_;
    uiSeisSel*		outfld_;
    uiGenInput*		interpfld_;
    uiLabeledSpinBox*	cellsizefld_;
    uiSeisSubSel*	subselfld_;

    bool		acceptOK(CallBacker*) override;
    void		cubeSel(CallBacker*);
    void		interpSelDone(CallBacker*);
};
