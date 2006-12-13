#ifndef uimaterialdlg_h
#define uimaterialdlg_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          April 2002
 RCS:           $Id: uimaterialdlg.h,v 1.8 2006-12-13 09:30:45 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "uigroup.h"
#include "draw.h"

class uiColorInput;
class uiSlider;
class uiSelLineStyle;
class uiTabStack;

namespace visBase { class Material; class VisualObject; };
namespace visSurvey { class SurveyObject; };


class uiPropertyGrp : public uiGroup
{
public:
    			uiPropertyGrp(uiParent* p, const char* nm)
			    : uiGroup(p,nm) {}

    virtual bool	rejectOK(CallBacker*)		{ return true; }
    virtual bool	acceptOK(CallBacker*)		{ return true; }
    virtual void	doFinalise(CallBacker*)		{}
};


class uiMaterialGrp : public uiPropertyGrp
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

    void			doFinalise(CallBacker*);
    void			sliderMove(CallBacker*);
    void			colorChangeCB(CallBacker*);
};


class uiLineStyleGrp : public uiPropertyGrp
{
public:
    				uiLineStyleGrp(uiParent*,
					       visSurvey::SurveyObject*);

protected:
    bool			rejectOK(CallBacker*);
    void			changedCB(CallBacker*);

    visSurvey::SurveyObject*	survobj_;
    LineStyle			backup_;
    uiSelLineStyle*		field_;
};


class uiPropertiesDlg : public uiDialog
{
public:
				uiPropertiesDlg(uiParent*,
						visSurvey::SurveyObject*);
protected:
    ObjectSet<uiPropertyGrp>	tabs_;
    uiTabStack*			tabstack_;

    visSurvey::SurveyObject*	survobj_;
    visBase::VisualObject*	visobj_;

    void			doFinalise(CallBacker*);
    bool			acceptOK(CallBacker*);
    bool			rejectOK(CallBacker*);
};

#endif
