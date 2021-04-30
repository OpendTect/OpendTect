#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        R. K. Singh
 Date:          May 2007
________________________________________________________________________

-*/

#include "uitutmod.h"
#include "uiattrdesced.h"

namespace Attrib { class Desc; }
class uiAttrSel;
class uiGenInput;
class uiSteeringSel;
class uiStepOutSel;


mExpClass(uiTut) uiTutorialAttrib : public uiAttrDescEd
{ mODTextTranslationClass(uiTutorialAttrib);
public:

			uiTutorialAttrib(uiParent*,bool);

protected:

    uiAttrSel*		inpfld_;
    uiGenInput*		actionfld_;
    uiGenInput*		factorfld_;
    uiGenInput*		shiftfld_;
    uiGenInput*		smoothstrengthfld_;
    uiGenInput*         smoothdirfld_;
    uiSteeringSel*      steerfld_;
    uiStepOutSel*       stepoutfld_;


    void		actionSel(CallBacker*);
    void                steerTypeSel(CallBacker*);

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);

    			mDeclReqAttribUIFns
};

