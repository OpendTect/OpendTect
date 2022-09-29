#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiattrdesced.h"

class uiAttrSel;
class uiGenInput;


/*! \brief MatchDelta Attribute description editor */

mClass(uiAttributes) uiMatchDeltaAttrib : public uiAttrDescEd
{ mODTextTranslationClass(uiMatchDeltaAttrib)
public:
			uiMatchDeltaAttrib(uiParent*,bool);
			~uiMatchDeltaAttrib();

    void		getEvalParams(TypeSet<EvalParam>&) const override;

protected:

    uiAttrSel*		refcubefld_;
    uiAttrSel*		mtchcubefld_;
    uiGenInput*		maxshiftfld_;

    bool		setParameters(const Attrib::Desc&) override;
    bool		setInput(const Attrib::Desc&) override;

    bool		getParameters(Attrib::Desc&) override;
    bool		getInput(Attrib::Desc&) override;

    			mDeclReqAttribUIFns
};
