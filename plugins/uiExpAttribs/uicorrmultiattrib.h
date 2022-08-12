#pragma once
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:       Rahul Gogia
 Date:	       June 2019
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
