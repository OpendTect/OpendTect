#ifndef uivisdirlightdlg_h
#define uivisdirlightdlg_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Karthika
 Date:          Sep 2009
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uivismod.h"
#include "uivismod.h"
#include "uidialog.h"
#include "uipolardiagram.h"

class uiDialExtra;
class uiGenInput;
class uiLabeledComboBox;
class uiPushButton;
class uiSliderExtra;
class uiVisPartServer;

namespace visBase { class DirectionalLight; }

mClass(uiVis) uiDirLightDlg : public uiDialog
{
public:
				uiDirLightDlg(uiParent*, uiVisPartServer*);
				~uiDirLightDlg();

    float			getHeadOnIntensity() const;
    void			setHeadOnIntensity(float);
    void			show();

protected:

    visBase::DirectionalLight*	getDirLight(int) const;
    void			setDirLight();
    float			getHeadOnLight(int) const;
    void			setHeadOnLight();
    float			getAmbientLight(int) const;
    bool			updateSceneSelector();	
    void			updateInitInfo();
    void			saveInitInfo();
    void			resetWidgets();
    void			setWidgets(bool);
    void			showWidgets(bool);
    void			validateInput();
    bool			isInSync();
    void			removeSceneNotifiers();

    bool			acceptOK(CallBacker*);
    bool			rejectOK(CallBacker*);
    void			pdDlgDoneCB(CallBacker*);
    void			showPolarDiagramCB(CallBacker*);
    void			lightSelChangedCB(CallBacker*);
    void			sceneSelChangedCB(CallBacker*);
    void			fieldChangedCB(CallBacker*);
    void			polarDiagramCB(CallBacker*);
    void			headOnChangedCB(CallBacker*);
    void			ambientChangedCB(CallBacker*);
    void			nrScenesChangedCB(CallBacker*);
    void			sceneNameChangedCB(CallBacker*);
    void			activeSceneChangedCB(CallBacker*);
    void			onOffChg(CallBacker*);
    void			setlightSwitch();
    
    uiVisPartServer*		visserv_;

    uiLabeledComboBox*		scenefld_;
    uiDialExtra*		azimuthfld_;
    uiSliderExtra*		dipfld_;
    uiSliderExtra*		intensityfld_;
    uiSliderExtra*		headonintensityfld_;
    uiSliderExtra*		ambintensityfld_;
    uiPushButton*		showpdfld_;
    uiPolarDiagram*		pd_;
    uiDialog*			pddlg_;
    uiGenInput*			lighttypefld_;
    uiGenInput*			switchfld_;

    typedef mStruct(uiVis) InitInfo
    {
		int		sceneid_;
        	
        	float		azimuth_;  // user degrees
	        float		dip_;  // degrees
        	
	        float		headonintensity_;
        	float		ambintensity_;
		
		bool		directlighton_;
		float		intensity_;
		float		dx_;
		float		dy_;
		float		dz_;
	
	public:

				InitInfo();
		void		reset(bool resetheadonval=true);

        	InitInfo& 	operator = (const InitInfo&);
        	bool		operator == (const InitInfo&) const;
        	bool	 	operator != (const InitInfo&) const;

    } InitInfoType;

    TypeSet<InitInfoType>	initinfo_;

    bool			initlighttype_;
    				// initial light type: 0 - headon light, 
    				// 1 - scene (directional light)
};

#endif


