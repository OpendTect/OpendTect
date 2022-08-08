#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		July 2016
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "uiattrdesced.h"

namespace Attrib { class Desc; }

class uiAttrSel;
class uiGenInput;

/*!
\brief Pseudo %Relief Attribute description editor
*/


mExpClass(uiAttributes) uiReliefAttrib : public uiAttrDescEd
{ mODTextTranslationClass(uiReliefAttrib)
public:
			uiReliefAttrib(uiParent*,bool);

protected:

    uiAttrSel*		inpfld_;
    uiGenInput*		gatefld_;

    bool		setParameters(const Attrib::Desc&) override;
    bool		setInput(const Attrib::Desc&) override;
    bool		setOutput(const Attrib::Desc&) override;

    bool		getParameters(Attrib::Desc&) override;
    bool		getInput(Attrib::Desc&) override;
    bool		getOutput(Attrib::Desc&) override;

			mDeclReqAttribUIFns
};

