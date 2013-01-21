#ifndef uicoherencyattrib_h
#define uicoherencyattrib_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          April 2001
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "uiattrdesced.h"

namespace Attrib { class Desc; };

class uiAttrSel;
class uiImagAttrSel;
class uiGenInput;
class uiStepOutSel;


/*! \brief Coherency attribute description editor */

mExpClass(uiAttributes) uiCoherencyAttrib : public uiAttrDescEd
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

