#ifndef uigapdeconattrib_h
#define uigapdeconattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Huck
 Date:          July 2006
 RCS:           $Id: uigapdeconattrib.h,v 1.2 2006-07-20 15:24:48 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiattrdesced.h"

class uiAttrSel;
class uiGenInput;
class uiLabeledSpinBox;
class uiPushButton;


/*! \brief GapDecon Attribute description editor */

class uiGapDeconAttrib : public uiAttrDescEd
{
public:

			uiGapDeconAttrib(uiParent*);

    void		getEvalParams(TypeSet<EvalParam>&) const;

protected:

    uiAttrSel*		inpfld_;
    uiGenInput*		gatefld_;
    uiGenInput*		lagfld_;
    uiGenInput*		gapfld_;
    uiGenInput*		noiselvlfld_;
    uiGenInput*		isinpzerophasefld_;
    uiGenInput*		isoutzerophasefld_;
    uiLabeledSpinBox*	stepoutfld_;
    uiPushButton*	exambut_;

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);

    void                examPush(CallBacker*);
};


#endif
