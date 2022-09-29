#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "uiattrdesced.h"

namespace Attrib { class Desc; };

class uiAttrSel;
class uiGenInput;
class uiStepOutSel;
class uiSteeringSel;

/*! \brief Shift Attribute description editor */

mExpClass(uiAttributes) uiShiftAttrib : public uiAttrDescEd
{ mODTextTranslationClass(uiShiftAttrib);
public:
			uiShiftAttrib(uiParent*,bool);
			~uiShiftAttrib();

    void		getEvalParams(TypeSet<EvalParam>&) const override;

protected:

    uiAttrSel*          inpfld_;
    uiStepOutSel*	stepoutfld_;
    uiGenInput*		timefld_;
    uiGenInput*		dosteerfld_;
    uiSteeringSel*	steerfld_;

    void		steerSel(CallBacker*);
    void                steerTypeSel(CallBacker*);
    bool		setParameters(const Attrib::Desc&) override;
    bool		setInput(const Attrib::Desc&) override;

    bool		getParameters(Attrib::Desc&) override;
    bool		getInput(Attrib::Desc&) override;

    			mDeclReqAttribUIFns
};
