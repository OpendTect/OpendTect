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

    void                getEvalParams(TypeSet<EvalParam>&) const;

protected:

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);
    DataPack::FullID	getInputDPID(int inpidx) const;
    void		updateNonSpecInputs();

    void		formSel(CallBacker*);
    void		inpSel(CallBacker*);
    void		rockPhysReq(CallBacker*);

    Math::Formula&	form_;
    uiMathFormula*	formfld_;

			mDeclReqAttribUIFns
};

