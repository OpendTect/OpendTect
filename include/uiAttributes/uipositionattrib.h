#ifndef uiposattrib_h
#define uiposattrib_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          November 2002
 RCS:           $Id: uipositionattrib.h,v 1.8 2009-07-22 16:01:20 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiattrdesced.h"

namespace Attrib { class Desc; }
class uiAttrSel;
class uiGenInput;
class uiSteeringSel;
class uiStepOutSel;

/*! \brief Position Attribute description editor */

mClass uiPositionAttrib : public uiAttrDescEd
{
public:

			uiPositionAttrib(uiParent*,bool);

    void		getEvalParams(TypeSet<EvalParam>&) const;

protected:

    uiAttrSel*          inpfld;
    uiAttrSel*          outfld;
    uiGenInput*		operfld;
    uiStepOutSel*	stepoutfld;
    uiGenInput*		gatefld;
    uiSteeringSel*	steerfld;

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);

    			mDeclReqAttribUIFns
};

#endif
