#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiwellattribmod.h"
#include "uiattrdesced.h"
#include "uistring.h"

class uiGenInput;
class uiListBox;
class uiWellSel;

namespace Attrib { class Desc; }

/*! \brief Energy Attribute ui */

mExpClass(uiWellAttrib) uiWellLogAttrib : public uiAttrDescEd
{ mODTextTranslationClass(uiWellLogAttrib);
public:

			uiWellLogAttrib(uiParent*,bool);

    void		getEvalParams(TypeSet<EvalParam>&) const override;

protected:

    void		selDone(CallBacker*);

    bool		setParameters(const Attrib::Desc&) override;
    bool		setInput(const Attrib::Desc&) override;
    bool		setOutput(const Attrib::Desc&) override;

    bool		getParameters(Attrib::Desc&) override;
    bool		getInput(Attrib::Desc&) override;
    bool		getOutput(Attrib::Desc&) override;

    uiWellSel*		wellfld_;
    uiListBox*		logsfld_;
    uiGenInput*		sampfld_;

			mDeclReqAttribUIFns
};
