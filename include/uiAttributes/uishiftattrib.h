#ifndef uishiftattrib_h
#define uishiftattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          October 2001
 RCS:           $Id: uishiftattrib.h,v 1.3 2005-08-22 15:33:53 cvsnanne Exp $
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

			uiShiftAttrib(uiParent*);
    void		getEvalParams(TypeSet<EvalParam>& params) const;

protected:

    uiAttrSel*          inpfld;
    uiStepOutSel*	stepoutfld;
    uiGenInput*		typefld;
    uiGenInput*		timefld;
    uiSteeringSel*	steerfld;

    void		shiftSel(CallBacker*);
    virtual void	set2D(bool);

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);
};

#endif
