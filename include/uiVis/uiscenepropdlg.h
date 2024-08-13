#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uivismod.h"

#include "ui3dviewer.h"
#include "uidialog.h"
#include "uiosgutil.h"
#include "uistring.h"

#include "color.h"
#include "fontdata.h"
#include "trckeyzsampling.h"
#include "vissurvscene.h"

class uiCheckBox;
class uiColorInput;
class uiGenInput;
class uiGenInputDlg;
class uiSlider;

namespace visSurvey { class Scene; }

/*! Dialog for scene properties. */

mExpClass(uiVis) uiScenePropertyDlg : public uiDialog
{ mODTextTranslationClass(uiScenePropertyDlg);
public:
			uiScenePropertyDlg(uiParent*,const
					   ObjectSet<ui3DViewer>&,int);
			~uiScenePropertyDlg();

private:

    void		updateScene(visSurvey::Scene*);
    void		updateCB(CallBacker*);
    void		setOffsetCB(CallBacker*);
    void		selAnnotFontCB(CallBacker*);
    void		setAnnotScaleCB(CallBacker*);
    bool		rejectOK(CallBacker*) override;
    bool		acceptOK(CallBacker*) override;

    const ObjectSet<ui3DViewer>&viewers_;
    int				curvwridx_;
    WeakPtr<visSurvey::Scene>	scene_;
    bool			hadsurveybox_	= true;
    bool			hadannot_	= true;
    bool			hadannotscale_	= true;
    bool			hadannotgrid_	= true;
    bool			hadanimation_	= true;
    OD::WheelMode		wheeldisplaymode_;
    OD::Color			oldbgcolor_	= OD::Color::Anthracite();
    float			oldmarkersize_	= 5;
    OD::Color			oldmarkercolor_ = OD::Color::White();
    OD::Color			annotcolor_	= OD::Color::White();
    float			oldfactor_;
    float			oldunits_;
    FontData			oldfont_;
    TrcKeyZSampling		oldscale_;
    static bool			savestatus_;

    uiCheckBox*			annotfld_;
    uiCheckBox*			annotscalefld_;
    uiCheckBox*			annotgridfld_;
    uiCheckBox*			survboxfld_;
    uiColorInput*		bgcolfld_;
    uiSlider*			markersizefld_;
    uiColorInput*		markercolfld_;
    uiColorInput*		annotcolfld_;
    uiGenInputDlg*		separationdlg_	= nullptr;
    uiCheckBox*			animationfld_;
    uiGenInput*			wheelmodefld_;
};
