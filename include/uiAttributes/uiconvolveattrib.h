#ifndef uiconvolveattrib_h
#define uiconvolveattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene Huck
 Date:          Nov 2006
 RCS:           $Id: uiconvolveattrib.h,v 1.4 2006-11-21 16:23:43 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiattrdesced.h"

class uiAttrSel;
class uiGenInput;
class uiLabeledSpinBox;
class CtxtIOObj;
class uiIOObjSel;

/*! \brief Convolve Attribute description editor */

class uiConvolveAttrib : public uiAttrDescEd
{
public:

			uiConvolveAttrib(uiParent*);
			~uiConvolveAttrib();

    virtual void	getEvalParams(TypeSet<EvalParam>&) const;

protected:

    uiAttrSel*          inpfld_;
    uiLabeledSpinBox*	szfld_;
    uiGenInput*         kernelfld_;
    uiGenInput*         shapefld_;
    uiGenInput*         outpfld_;
    uiIOObjSel*         waveletfld_;

    CtxtIOObj&          ctio_;

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);
    bool		setOutput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);
    bool		getOutput(Attrib::Desc&);

    virtual void	set2D(bool);

    void		kernelSel(CallBacker*);

    			mDeclReqAttribUIFns
};


#endif
