#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uivolumeprocessingmod.h"
#include "uivolprocchain.h"
#include "volprocsmoother.h"

class uiWindowFunctionSel;
class uiLabeledSpinBox;

namespace VolProc
{

mExpClass(uiVolumeProcessing) uiSmoother : public uiStepDialog
{ mODTextTranslationClass(uiSmoother)
public:

    mDefaultFactoryInstanciationBase(
	    Smoother::sFactoryKeyword(),
	    Smoother::sFactoryDisplayName())
	    mDefaultFactoryInitClassImpl(uiStepDialog,createInstance)

				~uiSmoother();

protected:
				uiSmoother(uiParent*,Smoother*,bool is2d);

    static uiStepDialog*	createInstance(uiParent*,Step*,bool is2d);
    bool			acceptOK(CallBacker*) override;
    void			updateFlds(CallBacker*);

    Smoother*			smoother_;

    uiWindowFunctionSel*	operatorselfld_;
    uiLabeledSpinBox*		inllenfld_;
    uiLabeledSpinBox*		crllenfld_;
    uiLabeledSpinBox*		zlenfld_;

};

} // namespace VolProc
