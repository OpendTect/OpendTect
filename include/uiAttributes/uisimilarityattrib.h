#ifndef uisimilarityattrib_h
#define uisimilarityattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2005
 RCS:           $Id: uisimilarityattrib.h,v 1.3 2006-09-11 07:04:12 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiattrdesced.h"

class uiAttrSel;
class uiGenInput;
class uiStepOutSel;
class uiSteeringSel;


/*! \brief Similarity Attribute description editor */

class uiSimilarityAttrib : public uiAttrDescEd
{
public:
    static void		initClass();
			uiSimilarityAttrib(uiParent*);

    void		set2D(bool);
    const char*		getAttribName() const;
    void		getEvalParams(TypeSet<EvalParam>&) const;

protected:
    static uiAttrDescEd* createInstance(uiParent*);

    uiAttrSel*		inpfld;
    uiSteeringSel*	steerfld;
    uiGenInput*		gatefld;
    uiGenInput*		extfld;
    uiStepOutSel*	pos0fld;
    uiStepOutSel*	pos1fld;
    uiStepOutSel*	stepoutfld;
    uiGenInput*		outpstatsfld;

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);
    bool		setOutput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);
    bool		getOutput(Attrib::Desc&);

    void		extSel(CallBacker*);
};


#endif
