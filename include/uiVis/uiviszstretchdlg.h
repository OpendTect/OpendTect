#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          April 2002
________________________________________________________________________

-*/

#include "uivismod.h"
#include "uidialog.h"

namespace visSurvey
{ class Scene; }

class uiCheckBox;
class uiLabeledComboBox;
class uiSlider;

/*!Dialog to set the z-stretch of a scene. */

mExpClass(uiVis) uiZStretchDlg : public uiDialog
{ mODTextTranslationClass(uiZStretchDlg);
public:
			uiZStretchDlg(uiParent*);
			~uiZStretchDlg();

    bool		valueChanged() const	{ return valchgd_; }

    CallBack		vwallcb; //!< If not set -> no button
    CallBack		homecb; //!< If not set -> no button

protected:

    uiLabeledComboBox*	scenefld_;
    uiSlider*		sliderfld_;
    uiCheckBox*		savefld_;
    uiButton*		vwallbut_;

    TypeSet<SceneID>	sceneids_;
    TypeSet<float>	zstretches_;
    TypeSet<float>	initzstretches_;


    static uiString	sZStretch() { return tr( "Z stretch" ); }
    void		setZStretch(visSurvey::Scene*,float,bool permanent);
    void		setOneZStretchToAllScenes(float,bool permanent);
    void		setZStretchesToScenes(TypeSet<float>&,bool permanent);

    void		updateSliderValues(int);
    visSurvey::Scene*	getSelectedScene() const;
    float		getSelectedSceneZStretch() const;
    float		getSelectedSceneUiFactor() const;

    void		doFinalize(CallBacker*);
    bool		acceptOK(CallBacker*);
    bool		rejectOK(CallBacker*);
    void		sliderMove(CallBacker*);
    void		butPush(CallBacker*);
    void		sceneSel(CallBacker*);

    float		initslval_;
			//!< will be removed after 6.0
    float		uifactor_;
			//!< will be removed after 6.0
    bool		valchgd_;
			//!< will be removed after 6.0
    void		updateSliderValues();
			//!< will be removed after 6.0
    void		setZStretch(float,bool permanent);
			//!< will be removed after 6.0
};
