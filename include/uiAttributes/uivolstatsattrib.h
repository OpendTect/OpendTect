#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          April 2001
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

    void		getEvalParams(TypeSet<EvalParam>& params) const;

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

    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);
    bool		setOutput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);
    bool		getOutput(Attrib::Desc&);

    			mDeclReqAttribUIFns
};

