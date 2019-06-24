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
#include "uiattribpanel.h"
#include "dbkey.h"
#include "iopar.h"



namespace Attrib { class Desc; }

class uiGenInput;
class uiAttrSel;


mExpClass(uiExpAttribs) uiIntegratedTrace : public uiAttrDescEd
{ mODTextTranslationClass(uiIntegratedTrace)
public:

			uiIntegratedTrace(uiParent*,bool);

protected:

    uiAttrSel*		inpfld_;
    uiGenInput*		gatefld_;

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    uiRetVal		getInput(Attrib::Desc&);

    void		fillInSDDescParams(Attrib::Desc*) const;

			mDeclReqAttribUIFns
};
