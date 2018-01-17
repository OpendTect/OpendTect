#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Aug 2012
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "uiattrdesced.h"


/*! \brief SampleValue Attribute description editor */

mExpClass(uiAttributes) uiSampleValueAttrib : public uiAttrDescEd
{ mODTextTranslationClass(uiSampleValueAttrib)
public:

			uiSampleValueAttrib(uiParent*,bool);

protected:

    uiAttrSel*          inpfld_;

    bool		setInput(const Attrib::Desc&);
    uiRetVal		getInput(Attrib::Desc&);

			mDeclReqAttribUIFns
};
