#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Huck
 Date:          Sep 2009
________________________________________________________________________

-*/

#include "uimadagascarattribsmod.h"
#include "uiattrdesced.h"

class uiAttrSel;
class uiGenInput;
class uiStepOutSel;


/*! \brief Madagascar AGC Attribute description editor */

mClass(uiMadagascarAttribs) uiMadAGCAttrib : public uiAttrDescEd
{ mODTextTranslationClass(uiMadAGCAttrib);
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


