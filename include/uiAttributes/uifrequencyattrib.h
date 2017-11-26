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

    void		getEvalParams(TypeSet<EvalParam>&) const;

protected:

    uiImagAttrSel*	inpfld_;
    uiGenInput*		gatefld_;
    uiGenInput*		normfld_;
    uiWindowFunctionSel* winfld_;
    uiGenInput*		outpfld_;
    uiGenInput*		smoothspectrumfld_;

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);
    bool		setOutput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    uiRetVal		getInput(Attrib::Desc&);
    bool		getOutput(Attrib::Desc&);

    virtual uiRetVal        areUIParsOK();

			mDeclReqAttribUIFns
};
