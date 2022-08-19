#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "uiattrdesced.h"


/*! \brief SampleValue Attribute description editor */

mExpClass(uiAttributes) uiSampleValueAttrib : public uiAttrDescEd
{
public:

			uiSampleValueAttrib(uiParent*,bool);

protected:

    uiAttrSel*          inpfld_;

    bool		setInput(const Attrib::Desc&) override;
    bool		getInput(Attrib::Desc&) override;

    			mDeclReqAttribUIFns
};
