#ifndef uiscalingattrib_h
#define uiscalingattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          December 2004
 RCS:           $Id: uiscalingattrib.h,v 1.4 2006-09-11 07:04:12 cvsnanne Exp $
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
    static void		initClass();
			uiScalingAttrib(uiParent*);

    const char*		getAttribName() const;

protected:
    static uiAttrDescEd* createInstance(uiParent*);

    uiAttrSel*		inpfld;
    uiGenInput*		typefld;
    uiGenInput*		nfld;
    uiGenInput*		statsfld;
    uiTable*		table;

    void		typeSel(CallBacker*);
    void		statsSel(CallBacker*);

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);
};

#endif
