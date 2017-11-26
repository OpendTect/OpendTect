#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          December 2004
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "uiattrdesced.h"
#include "attribdescid.h"

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

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    uiRetVal		getInput(Attrib::Desc&);

    void		getEvalParams(TypeSet<EvalParam>&) const;

    uiRetVal		areUIParsOK();

			mDeclReqAttribUIFns
};
