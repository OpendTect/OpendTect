#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
