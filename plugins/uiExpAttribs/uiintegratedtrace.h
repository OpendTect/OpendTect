#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Khushnood
 Date:		June 2019
________________________________________________________________________

-*/

#include "uiexpattribsmod.h"
#include "uiattrdesced.h"

namespace Attrib { class Desc; }

class uiAttrSel;


mExpClass(uiExpAttribs) uiIntegratedTrace : public uiAttrDescEd
{ mODTextTranslationClass(uiIntegratedTrace)
public:

			uiIntegratedTrace(uiParent*,bool);

protected:

    uiAttrSel*		inpfld_;

    bool		setInput(const Attrib::Desc&) override;
    bool		getInput(Attrib::Desc&) override;

			mDeclReqAttribUIFns
};
