#ifndef uienergyattrib_h
#define uienergyattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2005
 RCS:           $Id: uienergyattrib.h,v 1.2 2005-08-22 15:33:53 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiattrdesced.h"

namespace Attrib { class Desc; };

class uiAttrSel;
class uiGenInput;


/*! \brief Enery Attribute description editor */

class uiEnergyAttrib : public uiAttrDescEd
{
public:

			uiEnergyAttrib(uiParent*);

    void		getEvalParams(TypeSet<EvalParam>&) const;

protected:

    uiAttrSel*		inpfld;
    uiGenInput*		gatefld;

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);
};

#endif
