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

    void		getEvalParams(TypeSet<EvalParam>&) const;

protected:

    void		selDone(CallBacker*);

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);
    bool                setOutput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);
    bool                getOutput(Attrib::Desc&);

    uiWellSel*		wellfld_;
    uiListBox*		logsfld_;
    uiGenInput*		sampfld_;

			mDeclReqAttribUIFns
};

