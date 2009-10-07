#ifndef uivisdirlightdlg_h
#define uivisdirlightdlg_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Karthika
 Date:          Sep 2009
 RCS:           $Id: uivisdirlightdlg.h,v 1.6 2009-10-07 15:59:12 cvskarthika Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "uipolardiagram.h"

class uiVisPartServer;
class uiSliderExtra;
class uiLabeledComboBox;
class uiGenInput;

namespace visBase { class DirectionalLight; }

mClass uiDirLightDlg : public uiDialog
{
public:
				uiDirLightDlg(uiParent*, uiVisPartServer*);
				~uiDirLightDlg();

    bool			valueChanged() const;
    float			getHeadOnIntensity() const;
    void			setHeadOnIntensity(float);

protected:

    visBase::DirectionalLight*	getDirLight(int) const;
    void			setDirLight();
    float			getHeadOnLight(int) const;
    void			setHeadOnLight();
    int				updateSceneSelector();	
    void			updateWidgetValues(bool);
    void			showWidgets(bool);
    void			validateInput();

    bool			acceptOK(CallBacker*);
    bool			rejectOK(CallBacker*);
    void			sceneSel(CallBacker*);
    void			fieldChangedCB(CallBacker*);
    void			polarDiagramCB(CallBacker*);
    void			headOnChangedCB(CallBacker*);
    void			nrScenesChangedCB(CallBacker*);
    void			activeSceneChangedCB(CallBacker*);
    
    uiVisPartServer*		visserv_;
    uiLabeledComboBox*		scenefld_;
    uiSliderExtra*		azimuthfld_;
    uiSliderExtra*		dipfld_;
    uiSliderExtra*		intensityfld_;
    uiSliderExtra*		headonintensityfld_;

    TypeSet<int>		sceneids_;
    bool			valchgd_;
    // angles are in user degrees
    float			initazimuthval_;
    float			initdipval_;
    float			initintensityval_;
    float			initheadonval_;

    uiPolarDiagram*		pd_;

};

#endif
