#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert & H.Huck
 Date:          Jan 2008
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "uiattrdesced.h"
#include "datapack.h"
#include "stattype.h"

namespace Attrib { class Desc; };

class uiCheckBox;
class uiGenInput;
class uiLabel;
class uiPreStackSel;
class uiVelSel;
namespace PreStack { class uiProcSel; class uiAngleCompGrp;
		     class AngleCompParams; class AngleComputer; }

/*! \brief PreStack Attribute ui */

mExpClass(uiAttributes) uiPreStackAttrib : public uiAttrDescEd
{ mODTextTranslationClass(uiPreStackAttrib);
public:

			uiPreStackAttrib(uiParent*,bool);
			~uiPreStackAttrib();

    void                getEvalParams(TypeSet<EvalParam>&) const;

    void	setDataPackInp(const TypeSet<DataPack::FullID>&);

protected:

    uiPreStackSel*			prestackinpfld_;
    uiGenInput*				dopreprocessfld_;
    PreStack::uiProcSel*		preprocsel_;
    uiGenInput*				calctypefld_;
    uiGenInput*				stattypefld_;
    uiGenInput*				lsqtypefld_;
    uiCheckBox*				useanglefld_;
    uiGenInput*				gathertypefld_;
    uiGenInput*				xrgfld_;
    uiLabel*				xrglbl_;
    uiGenInput*				xunitfld_;
    uiGenInput*				xaxistypefld_;
    uiGenInput*				valaxtypefld_;
    void				updateCalcType();

    PreStack::uiAngleCompGrp*		anglecompgrp_;
    PreStack::AngleCompParams&		params_;
    EnumDef				statsdef_;

    bool		usedatapackasinput_;

    bool		setParameters(const Attrib::Desc&);
    bool		setAngleParameters(const Attrib::Desc&);
    bool		getParameters(Attrib::Desc&);
    bool		getAngleParameters(Attrib::Desc&);

    void		doPreProcSel(CallBacker*);
    void		calcTypSel(CallBacker*);
    void		angleTypSel(CallBacker*);
    void		gatherTypSel(CallBacker*);
    void		gatherUnitSel(CallBacker*);

    bool		setInput(const Attrib::Desc&);

			mDeclReqAttribUIFns
};
