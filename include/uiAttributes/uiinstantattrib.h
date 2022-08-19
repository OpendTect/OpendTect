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

class uiImagAttrSel;
class uiGenInput;
class uiLabeledSpinBox;

/*! \brief Instantaneous Attribute description editor */

mExpClass(uiAttributes) uiInstantaneousAttrib : public uiAttrDescEd
{ mODTextTranslationClass(uiInstantaneousAttrib);
public:

			uiInstantaneousAttrib(uiParent*,bool);

protected:

    uiImagAttrSel*	inpfld;
    uiGenInput*		outpfld;
    uiLabeledSpinBox*	phaserotfld;

    bool		setParameters(const Attrib::Desc&) override;
    bool		setInput(const Attrib::Desc&) override;
    bool		setOutput(const Attrib::Desc&) override;

    bool		getParameters(Attrib::Desc&) override;
    bool		getInput(Attrib::Desc&) override;
    bool		getOutput(Attrib::Desc&) override;

    void		outputSelCB(CallBacker*);
    void		getEvalParams(
				TypeSet<EvalParam>& params) const override;

    			mDeclReqAttribUIFns
};
