#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
			~uiVolProcAttrib();

protected:

    uiIOObjSel*		setupfld_;

    bool		setParameters(const Attrib::Desc&) override;
    bool		getParameters(Attrib::Desc&) override;

			mDeclReqAttribUIFns
};
