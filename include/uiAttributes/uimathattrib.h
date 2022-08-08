#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          October 2001
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

