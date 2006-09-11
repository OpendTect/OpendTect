#ifndef uienergyattrib_h
#define uienergyattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2005
 RCS:           $Id: uienergyattrib.h,v 1.3 2006-09-11 07:04:12 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiattrdesced.h"

namespace Attrib { class Desc; };

class uiAttrSel;
class uiGenInput;

/*! \brief Energy Attribute ui */

class uiEnergyAttrib : public uiAttrDescEd
{
public:

    static void		initClass();
			uiEnergyAttrib(uiParent*);

    const char*		getAttribName() const;
    void		getEvalParams(TypeSet<EvalParam>&) const;

protected:
    static uiAttrDescEd* createInstance(uiParent*);

    uiAttrSel*		inpfld;
    uiGenInput*		gatefld;

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);
};

#endif
