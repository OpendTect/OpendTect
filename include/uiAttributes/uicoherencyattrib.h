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
class uiStepOutSel;


/*! \brief Coherency attribute description editor */

mExpClass(uiAttributes) uiCoherencyAttrib : public uiAttrDescEd
{
public:
			uiCoherencyAttrib(uiParent*,bool);
			~uiCoherencyAttrib();

protected:

    uiImagAttrSel*	inpfld;
    uiGenInput*		is1fld;
    uiGenInput*		tgfld;
    uiGenInput*		maxdipfld;
    uiGenInput*		deltadipfld;
    uiStepOutSel*	stepoutfld;

    void		is1Sel(CallBacker*);

    bool                setParameters(const Attrib::Desc&) override;
    bool                setInput(const Attrib::Desc&) override;

    bool                getParameters(Attrib::Desc&) override;
    bool                getInput(Attrib::Desc&) override;

    void		getEvalParams(TypeSet<EvalParam>&) const override;

			mDeclReqAttribUIFns
};
