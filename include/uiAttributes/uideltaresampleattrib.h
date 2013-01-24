#ifndef uideltaresampleattrib_h
#define uideltaresampleattrib_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          Oct 2006
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiattrdesced.h"

class uiAttrSel;
class uiGenInput;


/*! \brief DeltaResample Attribute description editor */

mClass(uiAttributes) uiDeltaResampleAttrib : public uiAttrDescEd
{
public:

			uiDeltaResampleAttrib(uiParent*,bool);

protected:

    uiAttrSel*		refcubefld_;
    uiAttrSel*		deltacubefld_;
    uiGenInput*		periodfld_;

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);

    			mDeclReqAttribUIFns
};


#endif
