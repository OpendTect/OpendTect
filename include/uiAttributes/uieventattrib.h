#pragma once

/*+
 ________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        H. Payraudeau
 Date:          February 2005
 ________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "uiattrdesced.h"

namespace Attrib { class Desc; }

class uiAttrSel;
class uiGenInput;

/*! \brief Event Attributes description editor */

mExpClass(uiAttributes) uiEventAttrib : public uiAttrDescEd
{ mODTextTranslationClass(uiEventAttrib);
public:

                        uiEventAttrib(uiParent*,bool);

protected:

    uiAttrSel*		inpfld_;
    uiGenInput*		issinglefld_;
    uiGenInput*		tonextfld_;
    uiGenInput*		outpfld_;
    uiGenInput*		evtypefld_;
    uiGenInput*		gatefld_;
    uiGenInput*		outampfld_;

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);
    bool		setOutput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    uiRetVal		getInput(Attrib::Desc&);
    bool		getOutput(Attrib::Desc&);

    void		isSingleSel(CallBacker*);
    void		isGateSel(CallBacker*);
    void		outAmpSel(CallBacker*);

			mDeclReqAttribUIFns
};
