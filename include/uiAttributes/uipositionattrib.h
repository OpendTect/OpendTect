#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "uiattrdesced.h"

namespace Attrib { class Desc; }
class uiAttrSel;
class uiGenInput;
class uiSteeringSel;
class uiStepOutSel;

/*! \brief Position Attribute description editor */

mExpClass(uiAttributes) uiPositionAttrib : public uiAttrDescEd
{ mODTextTranslationClass(uiPositionAttrib);
public:
			uiPositionAttrib(uiParent*,bool);
			~uiPositionAttrib();

    void		getEvalParams(TypeSet<EvalParam>&) const override;

protected:

    uiAttrSel*		inpfld_;
    uiAttrSel*		outfld_;
    uiGenInput*		operfld_;
    uiStepOutSel*	stepoutfld_;
    uiGenInput*		gatefld_;
    uiSteeringSel*	steerfld_;

    bool		setParameters(const Attrib::Desc&) override;
    bool		setInput(const Attrib::Desc&) override;

    bool		getParameters(Attrib::Desc&) override;
    bool		getInput(Attrib::Desc&) override;

    void		steerTypeSel(CallBacker*);

			mDeclReqAttribUIFns
};
