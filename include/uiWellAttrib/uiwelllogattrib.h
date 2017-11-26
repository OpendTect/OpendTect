#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May 2005
________________________________________________________________________

-*/

#include "uiwellattribmod.h"
#include "uiattrdesced.h"
#include "uistring.h"

class uiGenInput;
class uiListBox;
class uiWellSel;

namespace Attrib { class Desc; }

/*! \brief Energy Attribute ui */

mExpClass(uiWellAttrib) uiWellLogAttrib : public uiAttrDescEd
{ mODTextTranslationClass(uiWellLogAttrib);
public:

			uiWellLogAttrib(uiParent*,bool);

protected:

    void		selDone(CallBacker*);

    bool		setParameters(const Attrib::Desc&);
    bool		getParameters(Attrib::Desc&);

    uiWellSel*		wellfld_;
    uiListBox*		logsfld_;
    uiGenInput*		sampfld_;

			mDeclReqAttribUIFns
};
