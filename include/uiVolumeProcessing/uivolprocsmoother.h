#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		February 2008
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

				uiSmoother(uiParent*,Smoother*,bool is2d);

protected:

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

