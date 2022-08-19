#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "uiattrdesced.h"

namespace Attrib { class Desc; };

class uiGenInput;

/*! \brief Reference Attribute description editor */

mExpClass(uiAttributes) uiReferenceAttrib : public uiAttrDescEd
{ mODTextTranslationClass(uiReferenceAttrib);
public:

			uiReferenceAttrib(uiParent*,bool);

protected:

    uiGenInput*		outpfld3d_	= nullptr;
    uiGenInput*		outpfld2d_	= nullptr;

    bool		setParameters(const Attrib::Desc&) override;
    bool		setInput(const Attrib::Desc&) override;
    bool		setOutput(const Attrib::Desc&) override;

    bool		getOutput(Attrib::Desc&) override;
    bool		getInput(Attrib::Desc&) override;
    bool		getParameters(Attrib::Desc&) override;

			mDeclReqAttribUIFns
};
