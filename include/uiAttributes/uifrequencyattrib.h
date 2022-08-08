#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          July 2001
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "uiattrdesced.h"

namespace Attrib { class Desc; };
class uiGenInput;
class uiImagAttrSel;
class uiWindowFunctionSel;

/*! \brief Frequency Attribute description editor */

mExpClass(uiAttributes) uiFrequencyAttrib : public uiAttrDescEd
{ mODTextTranslationClass(uiFrequencyAttrib);
public:

			uiFrequencyAttrib(uiParent*,bool);

    void		getEvalParams(TypeSet<EvalParam>&) const override;

protected:

    uiImagAttrSel*	inpfld;
    uiGenInput*         gatefld;
    uiGenInput*		normfld;
    uiGenInput*		smoothspectrumfld_;
    uiWindowFunctionSel* winfld;
    uiGenInput*		outpfld;

    bool		setParameters(const Attrib::Desc&) override;
    bool		setInput(const Attrib::Desc&) override;
    bool		setOutput(const Attrib::Desc&) override;

    bool		getParameters(Attrib::Desc&) override;
    bool		getInput(Attrib::Desc&) override;
    bool		getOutput(Attrib::Desc&) override;

    bool		areUIParsOK() override;

    			mDeclReqAttribUIFns
};

