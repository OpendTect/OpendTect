#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uivismod.h"

#include "uidialog.h"
#include "vissurvscene.h"

class uiCheckBox;
class uiLabeledComboBox;
class uiSlider;
class uiVisPartServer;

/*!Dialog to set the z-stretch of a scene. */

mExpClass(uiVis) uiZStretchDlg : public uiDialog
{ mODTextTranslationClass(uiZStretchDlg);
public:
			uiZStretchDlg(uiParent*,uiVisPartServer*);
			~uiZStretchDlg();

    CallBack		vwallcb; //!< If not set -> no button
    CallBack		homecb; //!< If not set -> no button

protected:

    void		setZStretch(visSurvey::Scene*,float,bool permanent);
    void		setOneZStretchToAllScenes(float,bool permanent);
    void		setZStretchesToScenes(TypeSet<float>&,bool permanent);

    void		updateSliderValues(int);
    RefMan<visSurvey::Scene> getSceneByIdx(int) const;
    RefMan<visSurvey::Scene> getSelectedScene() const;
    float		getSelectedSceneZStretch() const;
    float		getSelectedSceneUiFactor() const;

    void		doFinalize(CallBacker*);
    bool		acceptOK(CallBacker*) override;
    bool		rejectOK(CallBacker*) override;
    void		sliderMove(CallBacker*);
    void		butPush(CallBacker*);
    void		sceneSel(CallBacker*);

    uiVisPartServer*	vispartserv_;
    uiLabeledComboBox*	scenefld_	= nullptr;
    uiSlider*		sliderfld_	= nullptr;
    uiCheckBox*		savefld_	= nullptr;
    uiButton*		vwallbut_	= nullptr;

    TypeSet<SceneID>	sceneids_;
    TypeSet<float>	zstretches_;
    TypeSet<float>	initzstretches_;

    static uiString	sZStretch() { return tr( "Z stretch" ); }
};
