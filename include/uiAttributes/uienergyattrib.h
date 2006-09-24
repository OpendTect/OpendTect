#ifndef uienergyattrib_h
#define uienergyattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2005
 RCS:           $Id: uienergyattrib.h,v 1.4 2006-09-24 13:55:16 cvshelene Exp $
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

    uiAttrSel*		inpfld_;
    uiGenInput*		gatefld_;
    uiGenInput*         outpfld_;

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);
    bool                setOutput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);
    bool                getOutput(Attrib::Desc&);
};

#endif
