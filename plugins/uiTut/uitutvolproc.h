#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	H. Huck
 Date:		March 2016
 RCS:		$Id$
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
	mDefaultFactoryInstantiationBase(
	    VolProc::TutOpCalculator::sFactoryKeyword(),
	    VolProc::TutOpCalculator::sFactoryDisplayName())
	    mDefaultFactoryInitClassImpl( uiStepDialog, createInstance );

				~uiTutOpCalculator();


private:

				uiTutOpCalculator(uiParent*,TutOpCalculator*,
						  bool is2d);
    static uiStepDialog*	createInstance(uiParent*,Step*,bool is2d);

    bool			acceptOK();
    void			typeSel(CallBacker*);

    TutOpCalculator*		opcalc_;

    uiGenInput*			typefld_;
    uiStepOutSel*		shiftfld_;
};

} // namespace VolProc
