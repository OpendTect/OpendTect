#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uivismod.h"
#include "uidialog.h"
#include "ui3dviewer.h"
#include "uistring.h"
#include "color.h"
#include "trckeyzsampling.h"
#include "fontdata.h"

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

protected:
    void		updateScene(visSurvey::Scene*);
    void		updateCB(CallBacker*);
    void		setOffsetCB(CallBacker*);
    void		selAnnotFontCB(CallBacker*);
    void		setAnnotScaleCB(CallBacker*);
    bool		rejectOK(CallBacker*);
    bool		acceptOK(CallBacker*);

    const ObjectSet<ui3DViewer>&viewers_;
    int				curvwridx_;
    visSurvey::Scene*		scene_;
    bool			hadsurveybox_;
    bool			hadannot_;
    bool			hadannotscale_;
    bool			hadannotgrid_;
    bool			hadanimation_;
    ui3DViewer::WheelMode	wheeldisplaymode_;
    OD::Color			oldbgcolor_;
    float			oldmarkersize_;
    OD::Color			oldmarkercolor_;
    OD::Color			annotcolor_;
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
    uiGenInputDlg*		separationdlg_;
    uiCheckBox*			animationfld_;
    uiGenInput*			wheelmodefld_;
};
