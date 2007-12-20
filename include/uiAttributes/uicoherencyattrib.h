#ifndef uicoherencyattrib_h
#define uicoherencyattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          April 2001
 RCS:           $Id: uicoherencyattrib.h,v 1.5 2007-12-20 08:21:32 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiattrdesced.h"

namespace Attrib { class Desc; };

class uiAttrSel;
class uiImagAttrSel;
class uiGenInput;
class uiStepOutSel;


/*! \brief Coherency attribute description editor */

class uiCoherencyAttrib : public uiAttrDescEd
{
public:

			uiCoherencyAttrib(uiParent*,bool);

protected:

    uiImagAttrSel*	inpfld;
    uiGenInput*		is1fld;
    uiGenInput*		tgfld;
    uiGenInput*		maxdipfld;
    uiGenInput*		deltadipfld;
    uiStepOutSel*	stepoutfld;

    void		is1Sel(CallBacker*);

    bool                setParameters(const Attrib::Desc&);
    bool                setInput(const Attrib::Desc&);

    bool                getParameters(Attrib::Desc&);
    bool                getInput(Attrib::Desc&);

    void		getEvalParams(TypeSet<EvalParam>&) const;

			mDeclReqAttribUIFns
};


#endif
