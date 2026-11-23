#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uivsgmod.h"

#include "uidialog.h"

#include "color.h"

class uiCheckBox;
class uiColorInput;
class uiGenInput;
class uiVSGViewer;

/*!\brief The settings of the scene of the VSG viewer: the background and
  annotation colors and the thumbwheels. The color bar has its own dialog,
  uiVSGColorBarDlg, and the axes are shown by the axis button.

  A change is applied to the viewer at once. Cancel puts back what was set
  when the dialog was opened.
*/

mExpClass(uiVSG) uiVSGSceneSettingsDlg : public uiDialog
{ mODTextTranslationClass(uiVSGSceneSettingsDlg);
public:
			uiVSGSceneSettingsDlg(uiParent*,uiVSGViewer&);
			~uiVSGSceneSettingsDlg();

private:

    void		colorCB(CallBacker*);
    void		gradientCB(CallBacker*);
    void		wheelModeCB(CallBacker*);
    void		wheelOptionsCB(CallBacker*);
    void		updateSensitivity();
    bool		rejectOK(CallBacker*) override;

    uiVSGViewer&	viewer_;
    const OD::Color	hadbgcolor_;
    const OD::Color	hadannotcolor_;
    const bool		hadgradient_;
    const int		hadwheelmode_;
    const bool		hadwheelgrow_;
    const bool		hadglasswheels_;

    uiColorInput*	bgcolfld_;
    uiCheckBox*		gradientfld_;
    uiColorInput*	annotcolfld_;
    uiGenInput*		wheelmodefld_;
    uiCheckBox*		wheelgrowfld_;
    uiGenInput*		wheellookfld_;
};
