#ifndef uiposattrib_h
#define uiposattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          November 2002
 RCS:           $Id: uipositionattrib.h,v 1.2 2005-07-28 10:53:49 cvshelene Exp $
________________________________________________________________________

-*/

#include "uiattrdesced.h"

namespace Attrib { class Desc; }
class uiAttrSel;
class uiGenInput;
class uiSteeringSel;
class uiStepOutSel;

/*! \brief Position Attribute description editor */

class uiPositionAttrib : public uiAttrDescEd
{
public:
			uiPositionAttrib(uiParent*);

protected:

    uiAttrSel*          inpfld;
    uiAttrSel*          outfld;
    uiGenInput*		operfld;
    uiStepOutSel*	stepoutfld;
    uiGenInput*		gatefld;
    uiSteeringSel*	steerfld;

    virtual void	set2D(bool);

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);
};

#endif
