#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Huck
 Date:          Nov 2006
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "uiattrdesced.h"

class uiAttrSel;
class uiGenInput;
class uiLabeledSpinBox;
class uiIOObjSel;

/*! \brief Convolve Attribute description editor */

mExpClass(uiAttributes) uiConvolveAttrib : public uiAttrDescEd
{ mODTextTranslationClass(uiConvolveAttrib);
public:

			uiConvolveAttrib(uiParent*,bool);
			~uiConvolveAttrib();

    void		getEvalParams(TypeSet<EvalParam>&) const override;

protected:

    uiAttrSel*          inpfld_;
    uiLabeledSpinBox*	szfld_;
    uiGenInput*         kernelfld_;
    uiGenInput*         shapefld_;
    uiGenInput*         outpfld_;
    uiIOObjSel*         waveletfld_;

    bool		setParameters(const Attrib::Desc&) override;
    bool		setInput(const Attrib::Desc&) override;
    bool		setOutput(const Attrib::Desc&) override;

    bool		getParameters(Attrib::Desc&) override;
    bool		getInput(Attrib::Desc&) override;
    bool		getOutput(Attrib::Desc&) override;

    void		kernelSel(CallBacker*);

    			mDeclReqAttribUIFns
};


