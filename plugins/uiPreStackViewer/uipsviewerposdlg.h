#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uidialog.h"

class uiLabel;
class uiSpinBox;
class uiCheckBox;
class uiPushButton;
class uiLabeledSpinBox;

namespace visSurvey { class PreStackDisplay; }

namespace PreStackView
{

mClass(uiPreStackViewer) uiViewer3DPositionDlg : public uiDialog
{ mODTextTranslationClass(uiViewer3DPositionDlg);
public:

			uiViewer3DPositionDlg(uiParent*,
					      visSurvey::PreStackDisplay&);
			~uiViewer3DPositionDlg();

    bool		is3D() const;
    bool		isInl() const;

protected:

    uiSpinBox*		posfld_;
    uiLabeledSpinBox*	stepfld_;
    uiCheckBox*		oobox_;
    uiCheckBox*		applybox_;
    uiPushButton*	applybut_;
    uiLabel*		steplbl_;
    uiString		ootxt_;

    void		updateFieldDisplay();

    void		ooBoxSel(CallBacker*);
    void		applBoxSel(CallBacker*);
    void		posChg(CallBacker*);
    void		renewFld(CallBacker*);
    void		sectionChangedCB(CallBacker*);

    void		stepCB(CallBacker*);
    void		atStart(CallBacker*);
    void		applyCB(CallBacker*);

    bool		rejectOK(CallBacker*);

    visSurvey::PreStackDisplay& viewer_;
};

} // namespace
