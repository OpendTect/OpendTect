#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uivismod.h"

#include "uidialog.h"
#include "uipolardiagram.h"
#include "vissurvscene.h"

class uiDialExtra;
class uiGenInput;
class uiLabeledComboBox;
class uiPushButton;
class uiSlider;
class uiVisPartServer;

namespace visBase { class Light; }

mExpClass(uiVis) uiDirLightDlg : public uiDialog
{ mODTextTranslationClass(uiDirLightDlg);
public:
				uiDirLightDlg(uiParent*,uiVisPartServer*);
				~uiDirLightDlg();

    void			show() override;

protected:

    RefMan<visSurvey::Scene>	getScene(int infoidx) const;
    visBase::Light*		getDirLight(int infoidx) const;
    void			setDirLight();
    float			getCameraLightIntensity( int ) const;
    void			setCameraLightIntensity();
    float			getCameraAmbientIntensity(int) const;
    bool			updateSceneSelector();
    void			updateInitInfo();
    void			saveInitInfo();
    void			resetWidgets();
    void			setWidgets(bool);
    void			showWidgets(bool);
    void			validateInput();
    bool			isInSync();
    float			getDiffuseIntensity() const;

    bool			acceptOK(CallBacker*) override;
    bool			rejectOK(CallBacker*) override;
    void			pdDlgDoneCB(CallBacker*);
    void			showPolarDiagramCB(CallBacker*);
    void			lightSelChangedCB(CallBacker*);
    void			sceneSelChangedCB(CallBacker*);
    void			fieldChangedCB(CallBacker*);
    void			polarDiagramCB(CallBacker*);
    void			cameraLightChangedCB(CallBacker*);
    void			cameraAmbientChangedCB( CallBacker* );
    void			nrScenesChangedCB(CallBacker*);
    void			sceneNameChangedCB(CallBacker*);
    void			activeSceneChangedCB(CallBacker*);
    void			onOffChg(CallBacker*);
    void			setlightSwitch();

    uiVisPartServer*		visserv_;

    uiLabeledComboBox*		scenefld_;
    uiDialExtra*		azimuthfld_;
    uiSlider*			dipfld_;
    uiSlider*			dirintensityfld_;
    uiSlider*			cameradirintensityfld_;
    uiSlider*			cameraambintensityfld_;
    uiPushButton*		showpdfld_;
    uiPolarDiagram*		pd_ = nullptr;
    uiDialog*			pddlg_;
    uiGenInput*			switchfld_;

    typedef mStruct(uiVis) InitInfo
    {
	SceneID		sceneid_;

	float		azimuth_;  // user degrees
	float		dip_;  // degrees

	float		cameraintensity_;
	float		ambintensity_;

	bool		directlighton_;
	float		dirintensity_;
	float		dx_;
	float		dy_;
	float		dz_;

			InitInfo();
			~InitInfo();
	void		reset(bool resetheadonval=true);

	InitInfo&	operator = (const InitInfo&);
	bool		operator == (const InitInfo&) const;
	bool		operator != (const InitInfo&) const;

    } InitInfoType;

    TypeSet<InitInfoType>	initinfo_;

};
