#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uivsgmod.h"

#include "uidialog.h"

class uiCheckBox;
class uiGenInput;
class uiSpinBox;
class uiVSGViewer;

/*!\brief The settings of the scene of the VSG viewer: the axes, the
  thumbwheels and the color bar.

  A change is applied to the viewer at once. Cancel puts back what was set
  when the dialog was opened.
*/

mExpClass(uiVSG) uiVSGSceneSettingsDlg : public uiDialog
{ mODTextTranslationClass(uiVSGSceneSettingsDlg);
public:
			uiVSGSceneSettingsDlg(uiParent*,uiVSGViewer&);
			~uiVSGSceneSettingsDlg();

private:

    void		axesCB(CallBacker*);
    void		wheelModeCB(CallBacker*);
    void		wheelOptionsCB(CallBacker*);
    void		colorBarCB(CallBacker*);
    void		colorBarPosCB(CallBacker*);
    void		colorBarSizeCB(CallBacker*);
    void		setColorBarSizeLimits();
    void		updateSensitivity();
    bool		rejectOK(CallBacker*) override;

    uiVSGViewer&	viewer_;
    const bool		hadaxes_;
    const bool		hadmodernaxes_;
    const int		hadwheelmode_;
    const bool		hadwheelgrow_;
    const bool		hadglasswheels_;
    const bool		hadcolorbar_;
    const int		hadcolorbarpos_;
    const int		hadcolorbarwidth_;
    const int		hadcolorbarheight_;

    uiCheckBox*		axesfld_;
    uiGenInput*		axesstylefld_;
    uiGenInput*		wheelmodefld_;
    uiCheckBox*		wheelgrowfld_;
    uiGenInput*		wheellookfld_;
    uiCheckBox*		colorbarfld_;
    uiGenInput*		colorbarposfld_;
    uiSpinBox*		colorbarwidthfld_;
    uiSpinBox*		colorbarheightfld_;
};
