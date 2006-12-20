#ifndef uienergyattrib_h
#define uienergyattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2005
 RCS:           $Id: uienergyattrib.h,v 1.6 2006-12-20 11:23:00 cvshelene Exp $
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

			uiEnergyAttrib(uiParent*,bool);

    void		getEvalParams(TypeSet<EvalParam>&) const;

protected:

    uiAttrSel*		inpfld_;
    uiGenInput*		gatefld_;
    uiGenInput*         outpfld_;

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);
    bool                setOutput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);
    bool                getOutput(Attrib::Desc&);

    			mDeclReqAttribUIFns
};

#endif
