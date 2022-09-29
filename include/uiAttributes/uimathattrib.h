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
class uiMathFormula;
class uiGenInput;
class uiPushButton;
namespace Math { class Formula; }

/*! \brief Math Attribute description editor */

mExpClass(uiAttributes) uiMathAttrib : public uiAttrDescEd
{ mODTextTranslationClass(uiMathAttrib);
public:
			uiMathAttrib(uiParent*,bool);
			~uiMathAttrib();

    void		getEvalParams(TypeSet<EvalParam>&) const override;

protected:

    bool		setParameters(const Attrib::Desc&) override;
    bool		setInput(const Attrib::Desc&) override;

    bool		getParameters(Attrib::Desc&) override;
    bool		getInput(Attrib::Desc&) override;
    DataPack::FullID	getInputDPID(int inpidx) const;
    void		updateNonSpecInputs();

    void		formSel(CallBacker*);
    void		inpSel(CallBacker*);
    void		rockPhysReq(CallBacker*);

    Math::Formula&	form_;
    uiMathFormula*	formfld_;

			mDeclReqAttribUIFns
};
