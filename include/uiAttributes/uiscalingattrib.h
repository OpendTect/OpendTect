#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "uiattrdesced.h"

namespace Attrib { class Desc; class EngineMan; class Processor; }
class uiParent;
class uiAttrSel;
class uiGenInput;
class uiPushButton;
class uiTable;


/*! \brief Scaling Attribute description editor */

mExpClass(uiAttributes) uiScalingAttrib : public uiAttrDescEd
{ mODTextTranslationClass(uiScalingAttrib);
public:
			uiScalingAttrib(uiParent*,bool);
			~uiScalingAttrib();

protected:

    uiParent*		parent_;
    uiAttrSel*		inpfld;
    uiGenInput*		typefld;
    uiGenInput*		nfld;
    uiGenInput*		statsfld;
    uiTable*		table;
    uiGroup*		tblgrp;
    uiGenInput*         windowfld;
    uiGenInput*         lowenergymute;
    uiGenInput*         sqrgfld;
    uiGenInput*         squrgfld;
    uiPushButton*	analysebut_;

    TypeSet<float>	zvals_;
    TypeSet<float>	scalefactors_;

    void		typeSel(CallBacker*);
    void		statsSel(CallBacker*);
    void		analyseCB(CallBacker*);

    bool		setParameters(const Attrib::Desc&) override;
    bool		setInput(const Attrib::Desc&) override;

    bool		getParameters(Attrib::Desc&) override;
    bool		getInput(Attrib::Desc&) override;

    void		getEvalParams(TypeSet<EvalParam>&) const override;

    bool		areUIParsOK() override;

    			mDeclReqAttribUIFns
};
