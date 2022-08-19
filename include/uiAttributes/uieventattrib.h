#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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

    bool		setParameters(const Attrib::Desc&) override;
    bool		setInput(const Attrib::Desc&) override;
    bool		setOutput(const Attrib::Desc&) override;

    bool		getParameters(Attrib::Desc&) override;
    bool		getInput(Attrib::Desc&) override;
    bool		getOutput(Attrib::Desc&) override;

    void		isSingleSel(CallBacker*);
    void		isGateSel(CallBacker*);
    void		outAmpSel(CallBacker*);

			mDeclReqAttribUIFns
};
