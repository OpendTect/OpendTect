#ifndef uimadagcattrib_h
#define uimadagcattrib_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Huck
 Date:          Sep 2009
 RCS:           $Id: uimadagcattrib.h,v 1.1 2009/10/27 15:55:06 cvshelene Exp $
________________________________________________________________________

-*/

#include "uiattrdesced.h"

class uiAttrSel;
class uiGenInput;
class uiStepOutSel;


/*! \brief Madagascar AGC Attribute description editor */

class uiMadAGCAttrib : public uiAttrDescEd
{
public:

			uiMadAGCAttrib(uiParent*,bool);

    void		getEvalParams(TypeSet<EvalParam>&) const;

protected:

    uiAttrSel*		inpfld_;
    uiGenInput*		nrrepeatfld_;
    uiGenInput*		smoothzradiusfld_;
    uiStepOutSel*	smoothradiusfld_;

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);

    			mDeclReqAttribUIFns
};


#endif
