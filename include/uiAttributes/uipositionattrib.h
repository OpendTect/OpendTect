#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          November 2002
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "uiattrdesced.h"

namespace Attrib { class Desc; }
class uiAttrSel;
class uiGenInput;
class uiSteeringSel;
class uiStepOutSel;

/*! \brief Position Attribute description editor */

mExpClass(uiAttributes) uiPositionAttrib : public uiAttrDescEd
{ mODTextTranslationClass(uiPositionAttrib);
public:

			uiPositionAttrib(uiParent*,bool);

    void		getEvalParams(TypeSet<EvalParam>&) const override;

protected:

    uiAttrSel*          inpfld;
    uiAttrSel*          outfld;
    uiGenInput*		operfld;
    uiStepOutSel*	stepoutfld;
    uiGenInput*		gatefld;
    uiSteeringSel*	steerfld;

    bool		setParameters(const Attrib::Desc&) override;
    bool		setInput(const Attrib::Desc&) override;

    bool		getParameters(Attrib::Desc&) override;
    bool		getInput(Attrib::Desc&) override;

    void                steerTypeSel(CallBacker*);

    			mDeclReqAttribUIFns
};

