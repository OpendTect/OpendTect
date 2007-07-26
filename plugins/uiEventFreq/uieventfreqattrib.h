#ifndef uieventfreqattrib_h
#define uieventfreqattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          Jul 2007
 RCS:           $Id: uieventfreqattrib.h,v 1.1 2007-07-26 16:35:22 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiattrdesced.h"

class uiAttrSel;


/*! \brief DeltaResample Attribute description editor */

class uiEventFreqAttrib : public uiAttrDescEd
{
public:

			uiEventFreqAttrib(uiParent*,bool);

protected:

    uiAttrSel*		inpfld_;

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);

    			mDeclReqAttribUIFns
};


#endif
