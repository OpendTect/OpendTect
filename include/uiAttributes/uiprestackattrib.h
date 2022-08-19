#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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

    void		getEvalParams(TypeSet<EvalParam>&) const override;

    void	setDataPackInp(const TypeSet<DataPack::FullID>&) override;

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

    bool		usedatapackasinput_;

    bool		setParameters(const Attrib::Desc&) override;
    bool		setAngleParameters(const Attrib::Desc&);
    bool		getParameters(Attrib::Desc&) override;
    bool		getAngleParameters(Attrib::Desc&);

    Stats::Type		getStatEnumfromString(const char* stattypename);
    const char*		getStringfromStatEnum(Stats::Type enm);
    void		getStatTypeNames(BufferStringSet& stattypenames);

    void		doPreProcSel(CallBacker*);
    void		calcTypSel(CallBacker*);
    void		angleTypSel(CallBacker*);
    void		gatherTypSel(CallBacker*);
    void		gatherUnitSel(CallBacker*);

    bool		setInput(const Desc&) override;

			mDeclReqAttribUIFns
};
