#ifndef uishiftattrib_h
#define uishiftattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          October 2001
 RCS:           $Id: uishiftattrib.h,v 1.2 2005-07-28 10:53:49 cvshelene Exp $
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
