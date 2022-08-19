#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiexpattribsmod.h"
#include "uiattrdesced.h"
#include "uidialog.h"
#include "uilistbox.h"

class uiIOObjSel;
class uiGenInput;

mExpClass(uiExpAttribs) uiCorrMultiAttrib : public uiAttrDescEd
{mODTextTranslationClass(uiCorrMultiAttrib);
public:

			uiCorrMultiAttrib(uiParent*,bool);
			~uiCorrMultiAttrib();

protected:

    bool		acceptOK();
    bool		checkAttribName() const;

    bool		setParameters(const Attrib::Desc&) override;
    bool		setInput(const Attrib::Desc&) override;

    bool		getParameters(Attrib::Desc&) override;
    bool		getInput(Attrib::Desc&) override;

    uiAttrSel*		inpfld_;
    uiAttrSel*		inpfld2_;
    uiGenInput*		outfld_;
    uiGenInput*		gatefld_;
			mDeclReqAttribUIFns;
};
