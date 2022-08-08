#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          Oct 2006
________________________________________________________________________

-*/

#include "uiattrdesced.h"

class uiAttrSel;
class uiGenInput;


/*! \brief DeltaResample Attribute description editor */

mClass(uiAttributes) uiDeltaResampleAttrib : public uiAttrDescEd
{ mODTextTranslationClass(uiDeltaResampleAttrib);
public:

			uiDeltaResampleAttrib(uiParent*,bool);

protected:

    uiAttrSel*		refcubefld_;
    uiAttrSel*		deltacubefld_;
    uiGenInput*		periodfld_;

    bool		setParameters(const Attrib::Desc&) override;
    bool		setInput(const Attrib::Desc&) override;

    bool		getParameters(Attrib::Desc&) override;
    bool		getInput(Attrib::Desc&) override;

    			mDeclReqAttribUIFns
};


