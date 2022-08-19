#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitutmod.h"

#include "uivolprocstepdlg.h"

#include "tutvolproc.h"

class uiGenInput;
class uiStepOutSel;

namespace VolProc
{


mExpClass(uiTut) uiTutOpCalculator : public uiStepDialog
{ mODTextTranslationClass(uiTutOpCalculator);
public:
	    mDefaultFactoryInstanciationBase(
		VolProc::TutOpCalculator::sFactoryKeyword(),
		VolProc::TutOpCalculator::sFactoryDisplayName())
		mDefaultFactoryInitClassImpl( uiStepDialog, createInstance );

				~uiTutOpCalculator();


private:

				uiTutOpCalculator(uiParent*,TutOpCalculator*);
    static uiStepDialog*	createInstance(uiParent*,Step*,bool);

    bool			acceptOK(CallBacker*) override;
    void			typeSel(CallBacker*);

    TutOpCalculator*		opcalc_;

    uiGenInput*			typefld_;
    uiStepOutSel*		shiftfld_;
};

} // namespace VolProc
