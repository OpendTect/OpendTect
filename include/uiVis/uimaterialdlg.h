#ifndef uimaterialdlg_h
#define uimaterialdlg_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          April 2002
 RCS:           $Id: uimaterialdlg.h,v 1.6 2005-08-17 17:37:04 cvskris Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "uigroup.h"
#include "draw.h"

class uiColorInput;
class uiSlider;
class uiSelLineStyle;

namespace visBase { class Material; class VisualObject; };
namespace visSurvey { class SurveyObject; };


class uiPropertyGrp : public uiGroup
{
public:
    			uiPropertyGrp(uiParent* p, const char* nm)
			    : uiGroup(p,nm) {}
    virtual bool	rejectOK( CallBacker* ) { return true; }
    virtual bool	acceptOK( CallBacker* ) { return true; }
    virtual void	doFinalise( CallBacker* ) {}
};


class uiMaterialGrp : public uiPropertyGrp
{
public:
			uiMaterialGrp(uiParent*,visSurvey::SurveyObject*,
				      bool ambience=true,
				      bool diffusecolor=true,
				      bool specularcolor=true,
				      bool emmissivecolor=true,
				      bool shininess=true,
				      bool transparency=true,
			       	      bool color=false	);

protected:

    visBase::Material*		material;
    visSurvey::SurveyObject*	survobj;

    uiColorInput*	colinp;

    uiSlider*		ambslider;
    uiSlider*		diffslider;
    uiSlider*		specslider;
    uiSlider*		emisslider;
    uiSlider*		shineslider;
    uiSlider*		transslider;

    void		doFinalise(CallBacker*);
    void		ambSliderMove(CallBacker*);
    void		diffSliderMove(CallBacker*);
    void		specSliderMove(CallBacker*);
    void		emisSliderMove(CallBacker*);
    void		shineSliderMove(CallBacker*);
    void		transSliderMove(CallBacker*);
    void		colorChangeCB(CallBacker*);
};


class uiLineStyleGrp : public uiPropertyGrp
{
public:
    				uiLineStyleGrp( uiParent*,
						visSurvey::SurveyObject* );

protected:
    bool			rejectOK(CallBacker*);
    void			changedCB(CallBacker*);

    visSurvey::SurveyObject*	survobj;
    LineStyle			backup;
    uiSelLineStyle*		field;
};


class uiPropertiesDlg : public uiDialog
{
public:
			uiPropertiesDlg(uiParent*,visSurvey::SurveyObject*);
protected:
    ObjectSet<uiPropertyGrp>	tabs;
    uiTabStack*			tabstack;

    visSurvey::SurveyObject*	survobj;
    visBase::VisualObject*	visobj;

    void			doFinalise(CallBacker*);
    bool			acceptOK(CallBacker*);
    bool			rejectOK(CallBacker*);
};

#endif
