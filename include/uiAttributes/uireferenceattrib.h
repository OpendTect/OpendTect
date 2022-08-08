#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        H. Payraudeau
 Date:          July 2005
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

