#ifndef uiconvolveattrib_h
#define uiconvolveattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene Huck
 Date:          Nov 2006
 RCS:           $Id: uiconvolveattrib.h,v 1.7 2009-06-16 04:34:24 cvsnageswara Exp $
________________________________________________________________________

-*/

#include "uiattrdesced.h"

class uiAttrSel;
class uiGenInput;
class uiLabeledSpinBox;
class uiIOObjSel;

/*! \brief Convolve Attribute description editor */

mClass uiConvolveAttrib : public uiAttrDescEd
{
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
    uiIOObjSel*         waveletfld_;

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);
    bool		setOutput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);
    bool		getOutput(Attrib::Desc&);

    void		kernelSel(CallBacker*);

    			mDeclReqAttribUIFns
};


#endif
