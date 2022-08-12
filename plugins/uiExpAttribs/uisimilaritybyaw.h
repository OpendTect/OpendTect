#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Haibin Di
 Date:          August 2013
 ________________________________________________________________________
-*/

#include "uiexpattribsmod.h"
#include "uiattrdesced.h"


namespace Attrib { class Desc; }

class uiAttrSel;
class uiGenInput;
class uiSteeringSel;
class uiStepOutSel;


mExpClass(uiExpAttribs) uiSimilaritybyAW : public uiAttrDescEd
{ mODTextTranslationClass(uiSimilaritybyAW);

public:
    			uiSimilaritybyAW(uiParent*,bool);

protected:

			mDeclReqAttribUIFns

    void		choiceSel(CallBacker*);
    void		steerTypeSel(CallBacker*);
 
    bool		setParameters(const Attrib::Desc&) override;
    bool		setInput(const Attrib::Desc&) override;
    bool		getParameters(Attrib::Desc&) override;
    bool		getInput(Attrib::Desc&) override;

    uiAttrSel*		inputfld_;
    uiSteeringSel*	steerfld_;
    uiGenInput*		reftimegatefld_;
    uiGenInput*		searchrangefld_;
    uiStepOutSel*	stepoutfld_;
    uiGenInput*		attributefld_;
};

