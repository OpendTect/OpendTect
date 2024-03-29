#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "uiattrdesced.h"

class uiAttrSel;
class uiGenInput;
class uiLabeledSpinBox;

/*! \brief DipFilter Attribute description editor */

mExpClass(uiAttributes) uiDipFilterAttrib : public uiAttrDescEd
{ mODTextTranslationClass(uiDipFilterAttrib);
public:
			uiDipFilterAttrib(uiParent*,bool);
			~uiDipFilterAttrib();

    void		getEvalParams(TypeSet<EvalParam>&) const override;

protected:

    uiAttrSel*		inpfld_;
    uiLabeledSpinBox*	szfld_;
    uiGenInput*		fltrtpfld_;
    uiGenInput*		velfld_;
    uiGenInput*		azifld_;
    uiGenInput*		aziintfld_;
    uiGenInput*		taperfld_;

    bool		setParameters(const Attrib::Desc&) override;
    bool		setInput(const Attrib::Desc&) override;

    bool		getParameters(Attrib::Desc&) override;
    bool		getInput(Attrib::Desc&) override;

    void		panelbutCB(CallBacker*);
    void		fkWinCloseCB(CallBacker*);
    void		filtSel(CallBacker*);
    void		aziSel(CallBacker*);

			mDeclReqAttribUIFns
};
