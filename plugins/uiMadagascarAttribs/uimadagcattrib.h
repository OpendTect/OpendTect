#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uimadagascarattribsmod.h"
#include "uiattrdesced.h"

class uiAttrSel;
class uiGenInput;
class uiStepOutSel;


/*! \brief Madagascar AGC Attribute description editor */

mClass(uiMadagascarAttribs) uiMadAGCAttrib : public uiAttrDescEd
{ mODTextTranslationClass(uiMadAGCAttrib);
public:

			uiMadAGCAttrib(uiParent*,bool);

    void		getEvalParams(TypeSet<EvalParam>&) const override;

protected:

    uiAttrSel*		inpfld_;
    uiGenInput*		nrrepeatfld_;
    uiGenInput*		smoothzradiusfld_;
    uiStepOutSel*	smoothradiusfld_;

    bool		setParameters(const Attrib::Desc&) override;
    bool		setInput(const Attrib::Desc&) override;

    bool		getParameters(Attrib::Desc&) override;
    bool		getInput(Attrib::Desc&) override;

    			mDeclReqAttribUIFns
};
