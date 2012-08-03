#ifndef uimaterialdlg_h
#define uimaterialdlg_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          April 2002
 RCS:           $Id: uimaterialdlg.h,v 1.15 2012-08-03 13:01:18 cvskris Exp $
________________________________________________________________________

-*/

#include "uivismod.h"
#include "uidialog.h"
#include "uidlggroup.h"
#include "draw.h"

class uiColorInput;
class uiGenInput;
class uiSlider;
class uiSelLineStyle;
class uiTabStack;

namespace visBase { class Material; class VisualObject; };
namespace visSurvey { class SurveyObject; };


mClass(uiVis) uiMaterialGrp : public uiDlgGroup
{
public:
				uiMaterialGrp(uiParent*,
					      visSurvey::SurveyObject*,
					      bool ambience=true,
					      bool diffusecolor=true,
					      bool specularcolor=true,
					      bool emmissivecolor=true,
					      bool shininess=true,
					      bool transparency=true,
					      bool color=false);

protected:
    void			createSlider(bool,uiSlider*&,const char*);

    visBase::Material*		material_;
    visSurvey::SurveyObject*	survobj_;

    uiColorInput*		colinp_;
    uiSlider*			ambslider_;
    uiSlider*			diffslider_;
    uiSlider*			specslider_;
    uiSlider*			emisslider_;
    uiSlider*			shineslider_;
    uiSlider*			transslider_;
    uiGroup*			prevobj_;

    void			sliderMove(CallBacker*);
    void			colorChangeCB(CallBacker*);
};


mClass(uiVis) uiLineStyleGrp : public uiDlgGroup
{
public:
    				uiLineStyleGrp(uiParent*,
					       visSurvey::SurveyObject*);

protected:

    virtual bool		rejectOK()	{ return rejectOK(0); }
    bool			rejectOK(CallBacker*);
    void			changedCB(CallBacker*);

    visSurvey::SurveyObject*	survobj_;
    LineStyle			backup_;
    uiSelLineStyle*		field_;

};


mClass(uiVis) uiTextureInterpolateGrp : public uiDlgGroup
{
public:
				uiTextureInterpolateGrp(uiParent*,
					visSurvey::SurveyObject*);
protected:				
    void			chgIntpCB(CallBacker*);
    
    uiGenInput*			textclasssify_;
    visSurvey::SurveyObject*	survobj_;
};


mClass(uiVis) uiPropertiesDlg : public uiTabStackDlg
{
public:
				uiPropertiesDlg(uiParent*,
						visSurvey::SurveyObject*);
protected:

    visSurvey::SurveyObject*	survobj_;
    visBase::VisualObject*	visobj_;
};

#endif

