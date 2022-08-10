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
#include "uidlggroup.h"
#include "draw.h"
#include "uistring.h"

class uiColorInput;
class uiGenInput;
class uiSlider;
class uiSelLineStyle;
class uiTabStack;
class uiMarkerStyle3D;

namespace visBase { class Material; }
namespace visSurvey { class SurveyObject; }


mExpClass(uiVis) uiMaterialGrp : public uiDlgGroup
{ mODTextTranslationClass(uiMaterialGrp);
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
				~uiMaterialGrp();

protected:
    void			createSlider(bool,uiSlider*&,const uiString&);

    visBase::Material*		material_;
    visSurvey::SurveyObject*	survobj_;

    uiColorInput*		colinp_		= nullptr;
    uiSlider*			ambslider_	= nullptr;
    uiSlider*			diffslider_	= nullptr;
    uiSlider*			specslider_	= nullptr;
    uiSlider*			emisslider_	= nullptr;
    uiSlider*			shineslider_	= nullptr;
    uiSlider*			transslider_	= nullptr;
    uiGroup*			prevobj_	= nullptr;

    void			sliderMove(CallBacker*);
    void			colorChangeCB(CallBacker*);
};


mExpClass(uiVis) uiLineStyleGrp : public uiDlgGroup
{ mODTextTranslationClass(uiLineStyleGrp);
public:
    				uiLineStyleGrp(uiParent*,
					       visSurvey::SurveyObject*);

protected:

    virtual bool		rejectOK()	{ return rejectOK(0); }
    bool			rejectOK(CallBacker*);
    void			changedCB(CallBacker*);

    visSurvey::SurveyObject*	survobj_;
    OD::LineStyle		backup_;
    uiSelLineStyle*		field_;

};


mExpClass(uiVis) uiTextureInterpolateGrp : public uiDlgGroup
{ mODTextTranslationClass(uiTextureInterpolateGrp);
public:
				uiTextureInterpolateGrp(uiParent*,
					visSurvey::SurveyObject*);
protected:
    void			chgIntpCB(CallBacker*);

    uiGenInput*			textclasssify_;
    visSurvey::SurveyObject*	survobj_;
};


mExpClass(uiVis) uiMarkerStyleGrp : public uiDlgGroup
{mODTextTranslationClass(uiMarkerStyleGrp);
public:
				uiMarkerStyleGrp(uiParent*,
					visSurvey::SurveyObject*);
				~uiMarkerStyleGrp();

protected:
    uiMarkerStyle3D*		stylefld_	= nullptr;
    visSurvey::SurveyObject*	survobj_;

    void			sizeChg(CallBacker*);
    void			typeSel(CallBacker*);
    void			colSel(CallBacker*);
};


mExpClass(uiVis) uiPropertiesDlg : public uiTabStackDlg
{ mODTextTranslationClass(uiPropertiesDlg);
public:
				uiPropertiesDlg(uiParent*,
						visSurvey::SurveyObject*);
				~uiPropertiesDlg();

    static CNotifier<uiPropertiesDlg,VisID>& closeNotifier();

protected:
    void			dlgClosed(CallBacker*);

    visSurvey::SurveyObject*	survobj_;
};
