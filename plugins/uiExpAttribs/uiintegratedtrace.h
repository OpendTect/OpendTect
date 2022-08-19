#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
