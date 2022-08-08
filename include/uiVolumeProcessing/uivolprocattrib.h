#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		December 2013
________________________________________________________________________

-*/

#include "uivolumeprocessingmod.h"
#include "uiattrdesced.h"
#include "uistring.h"

namespace Attrib { class Desc; };

class uiIOObjSel;

/*! \brief VolumeProcessing Attribute ui */

mExpClass(uiVolumeProcessing) uiVolProcAttrib : public uiAttrDescEd
{ mODTextTranslationClass(uiVolProcAttrib);
public:
			uiVolProcAttrib(uiParent*,bool);

protected:

    uiIOObjSel*		setupfld_;

    bool		setParameters(const Attrib::Desc&) override;
    bool		getParameters(Attrib::Desc&) override;

			mDeclReqAttribUIFns
};

