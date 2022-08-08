#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          Aug 2006
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


