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
class uiWaveletIOObjSel;

/*! \brief Convolve Attribute description editor */

mExpClass(uiAttributes) uiConvolveAttrib : public uiAttrDescEd
{ mODTextTranslationClass(uiConvolveAttrib);
public:

			uiConvolveAttrib(uiParent*,bool);
			~uiConvolveAttrib();

    virtual void	getEvalParams(TypeSet<EvalParam>&) const;

protected:

    uiAttrSel*          inpfld_;
    uiLabeledSpinBox*	szfld_;
    uiGenInput*         kernelfld_;
    uiGenInput*         shapefld_;
    uiGenInput*         outpfld_;
    uiWaveletIOObjSel*	waveletfld_;

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);
    bool		setOutput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    uiRetVal		getInput(Attrib::Desc&);
    bool		getOutput(Attrib::Desc&);

    void		kernelSel(CallBacker*);

			mDeclReqAttribUIFns
};
