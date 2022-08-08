#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          July 2001
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


