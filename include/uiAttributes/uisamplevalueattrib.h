#ifndef uisamplevalueattrib_h
#define uisamplevalueattrib_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Aug 2012
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiattrdesced.h"


/*! \brief SampleValue Attribute description editor */

mClass uiSampleValueAttrib : public uiAttrDescEd
{
public:

			uiSampleValueAttrib(uiParent*,bool);

protected:

    uiAttrSel*          inpfld_;

    bool		setInput(const Attrib::Desc&);
    bool		getInput(Attrib::Desc&);

    			mDeclReqAttribUIFns
};

#endif

