#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          Jan 2008
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "uiattrdesced.h"

class uiAttrSel;
class uiGenInput;
class uiStepOutSel;
class uiSteeringSel;


/*! \brief Semblance Attribute description editor */

mExpClass(uiAttributes) uiSemblanceAttrib : public uiAttrDescEd
{ mODTextTranslationClass(uiSemblanceAttrib);
public:

			uiSemblanceAttrib(uiParent*,bool);

    void		getEvalParams(TypeSet<EvalParam>&) const override;

protected:

    uiAttrSel*		inpfld;
    uiSteeringSel*	steerfld;
    uiGenInput*		gatefld;
    uiGenInput*		extfld;
    uiStepOutSel*	pos0fld;
    uiStepOutSel*	pos1fld;
    uiStepOutSel*	stepoutfld;

    bool		setParameters(const Attrib::Desc&) override;
    bool		setInput(const Attrib::Desc&) override;

    bool		getParameters(Attrib::Desc&) override;
    bool		getInput(Attrib::Desc&) override;

    void		extSel(CallBacker*);

    			mDeclReqAttribUIFns
};

