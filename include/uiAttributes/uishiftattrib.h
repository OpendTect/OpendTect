#ifndef uishiftattrib_h
#define uishiftattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          October 2001
 RCS:           $Id: uishiftattrib.h,v 1.6 2006-12-20 11:23:00 cvshelene Exp $
________________________________________________________________________

-*/

#include "uiattrdesced.h"

namespace Attrib { class Desc; };

class uiAttrSel;
class uiGenInput;
class uiStepOutSel;
class uiSteeringSel;

/*! \brief Shift Attribute description editor */

class uiShiftAttrib : public uiAttrDescEd
{
public:

			uiShiftAttrib(uiParent*,bool);

    void		getEvalParams(TypeSet<EvalParam>& params) const;

protected:

    uiAttrSel*          inpfld;
    uiStepOutSel*	stepoutfld;
    uiGenInput*		typefld;
    uiGenInput*		timefld;
    uiSteeringSel*	steerfld;

    void		shiftSel(CallBacker*);
    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);

    			mDeclReqAttribUIFns
};

#endif
