#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
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

    void		getEvalParams(TypeSet<EvalParam>&) const;

protected:

    uiAttrSel*		refcubefld_;
    uiAttrSel*		mtchcubefld_;
    uiGenInput*		maxshiftfld_;

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    uiRetVal		getInput(Attrib::Desc&);

			mDeclReqAttribUIFns
};
