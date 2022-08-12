#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          Jul 2007
________________________________________________________________________

-*/

#include "uiexpattribsmod.h"
#include "uiattrdesced.h"

class uiAttrSel;
class uiGenInput;


/*! \brief DeltaResample Attribute description editor */

mClass(uiExpAttribs) uiEventFreqAttrib : public uiAttrDescEd
{ mODTextTranslationClass(uiEventFreqAttrib);
public:

			uiEventFreqAttrib(uiParent*,bool);

protected:

    uiAttrSel*		inpfld_;
    uiGenInput*		typfld_;

    bool		setParameters(const Attrib::Desc&) override;
    bool		setInput(const Attrib::Desc&) override;
    bool		setOutput(const Attrib::Desc&) override;

    bool		getParameters(Attrib::Desc&) override;
    bool		getInput(Attrib::Desc&) override;
    bool		getOutput(Attrib::Desc&) override;

			mDeclReqAttribUIFns
};


