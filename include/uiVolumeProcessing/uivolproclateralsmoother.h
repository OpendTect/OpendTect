#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uivolumeprocessingmod.h"
#include "uivolprocchain.h"
#include "volproclateralsmoother.h"

class uiGenInput;
class uiLabeledSpinBox;

namespace VolProc
{

class LateralSmoother;


mExpClass(uiVolumeProcessing) uiLateralSmoother : public uiStepDialog
{ mODTextTranslationClass(uiLateralSmoother);
public:
    mDefaultFactoryInstanciationBase(
	VolProc::LateralSmoother::sFactoryKeyword(),
	VolProc::LateralSmoother::sFactoryDisplayName())
	mDefaultFactoryInitClassImpl( uiStepDialog, createInstance );


protected:

				uiLateralSmoother(uiParent*,LateralSmoother*,
						  bool is2d);
    static uiStepDialog*	createInstance(uiParent*, Step*,bool is2d);

    bool			acceptOK(CallBacker*) override;
    void			updateFlds(CallBacker*);

    LateralSmoother*		smoother_;

    uiLabeledSpinBox*		inllenfld_;
    uiLabeledSpinBox*		crllenfld_;
    uiGenInput*			replaceudfsfld_;

    uiGenInput*			ismedianfld_;
    uiGenInput*			weightedfld_;
    uiGenInput*			mirroredgesfld_;

    uiGenInput*			udfhandling_;
    uiGenInput*			udffixedvalue_;


};

} // namespace VolProc
