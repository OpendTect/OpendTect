#ifndef uiscalingattrib_h
#define uiscalingattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          December 2004
 RCS:           $Id: uiscalingattrib.h,v 1.8 2008-12-05 04:35:24 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiattrdesced.h"

namespace Attrib { class Desc; }
class uiAttrSel;
class uiGenInput;
class uiTable;


/*! \brief Scaling Attribute description editor */

class uiScalingAttrib : public uiAttrDescEd
{
public:

			uiScalingAttrib(uiParent*,bool);

protected:

    uiAttrSel*		inpfld;
    uiGenInput*		typefld;
    uiGenInput*		nfld;
    uiGenInput*		statsfld;
    uiTable*		table;
    uiGenInput*         windowfld;
    uiGenInput*         lowenergymute;

    void		typeSel(CallBacker*);
    void		statsSel(CallBacker*);

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);

    void		getEvalParams(TypeSet<EvalParam>&) const;

    			mDeclReqAttribUIFns
};

#endif
