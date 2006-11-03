#ifndef uiconvolveattrib_h
#define uiconvolveattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene Huck
 Date:          Nov 2006
 RCS:           $Id: uiconvolveattrib.h,v 1.1 2006-11-03 16:01:36 cvshelene Exp $
________________________________________________________________________

-*/

#include "uiattrdesced.h"

class uiAttrSel;
class uiGenInput;
class uiLabeledSpinBox;

/*! \brief DipFilter Attribute description editor */

class uiConvolveAttrib : public uiAttrDescEd
{
public:

			uiConvolveAttrib(uiParent*);

    virtual void	getEvalParams(TypeSet<EvalParam>&) const;

protected:

    uiAttrSel*          inpfld;
    uiLabeledSpinBox*	szfld;
    uiGenInput*         kernelfld;
    uiGenInput*         shapefld;
    uiGenInput*         outpfld;

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
