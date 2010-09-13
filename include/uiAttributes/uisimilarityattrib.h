#ifndef uisimilarityattrib_h
#define uisimilarityattrib_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May 2005
 RCS:           $Id: uisimilarityattrib.h,v 1.8 2010-09-13 14:10:33 cvshelene Exp $
________________________________________________________________________

-*/

#include "uiattrdesced.h"

class uiAttrSel;
class uiGenInput;
class uiStepOutSel;
class uiSteeringSel;


/*! \brief Similarity Attribute description editor */

mClass uiSimilarityAttrib : public uiAttrDescEd
{
public:

			uiSimilarityAttrib(uiParent*,bool);

    void		getEvalParams(TypeSet<EvalParam>&) const;

protected:

    uiAttrSel*		inpfld_;
    uiSteeringSel*	steerfld_;
    uiGenInput*		gatefld_;
    uiGenInput*		extfld_;
    uiStepOutSel*	pos0fld_;
    uiStepOutSel*	pos1fld_;
    uiStepOutSel*	stepoutfld_;
    uiGenInput*		outpstatsfld_;
    uiGenInput*		maxdipfld_;
    uiGenInput*		deltadipfld_;

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);
    bool		setOutput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);
    bool		getOutput(Attrib::Desc&);

    void		extSel(CallBacker*);

    			mDeclReqAttribUIFns
};


#endif
