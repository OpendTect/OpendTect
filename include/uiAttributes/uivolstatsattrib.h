#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "uiattrdesced.h"

namespace Attrib { class Desc; }

class uiGenInput;
class uiAttrSel;
class uiCheckBox;
class uiLabeledSpinBox;
class uiSteeringSel;
class uiStepOutSel;


/*! \brief VolumeStatistics Attribute description editor */

mExpClass(uiAttributes) uiVolumeStatisticsAttrib : public uiAttrDescEd
{ mODTextTranslationClass(uiVolumeStatisticsAttrib);
public:
			uiVolumeStatisticsAttrib(uiParent*,bool);
			~uiVolumeStatisticsAttrib();

    void		getEvalParams(TypeSet<EvalParam>&) const override;

protected:

    uiAttrSel*		inpfld_;
    uiSteeringSel*	steerfld_;
    uiGenInput*		gatefld_;
    uiGenInput*		shapefld_;
    uiStepOutSel*	stepoutfld_;
    uiLabeledSpinBox*	nrtrcsfld_;
    uiGenInput*		outpfld_;

    uiLabeledSpinBox*	optstackstepfld_;
    uiCheckBox*		edgeeffectfld_;
    uiGenInput*		stackdirfld_;

    void		stackstepChg(CallBacker*);
    void		stepoutChg(CallBacker*);
    void		shapeChg(CallBacker*);

    bool		setParameters(const Attrib::Desc&) override;
    bool		setInput(const Attrib::Desc&) override;
    bool		setOutput(const Attrib::Desc&) override;

    bool		getParameters(Attrib::Desc&) override;
    bool		getInput(Attrib::Desc&) override;
    bool		getOutput(Attrib::Desc&) override;

    			mDeclReqAttribUIFns
};
