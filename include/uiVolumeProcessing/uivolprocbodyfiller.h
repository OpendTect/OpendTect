#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Y.C. Liu
 Date:		November 2007
________________________________________________________________________

-*/

#include "uivolumeprocessingmod.h"
#include "uivolprocstepdlg.h"
#include "volprocbodyfiller.h"

class uiBodySel;

namespace VolProc
{

mExpClass(uiVolumeProcessing) uiBodyFiller: public uiStepDialog
{ mODTextTranslationClass(uiBodyFiller);
public:
	mDefaultFactoryInstanciationBase(
		VolProc::BodyFiller::sFactoryKeyword(),
		VolProc::BodyFiller::sFactoryDisplayName());

				~uiBodyFiller();

protected:
				uiBodyFiller(uiParent*,BodyFiller*,bool is2d);
    static uiStepDialog*	createInstance(uiParent*,Step*,bool is2d);

    bool			acceptOK(CallBacker*) override;
    void			bodySel(CallBacker*);
    void			typeSel(CallBacker*);

    BodyFiller*			bodyfiller_;

    uiBodySel*			bodyfld_;
    uiGenInput*			insidetypfld_;
    uiGenInput*			insidevaluefld_;
    uiGenInput*			outsidetypfld_;
    uiGenInput*			outsidevaluefld_;
};

} // namespace VolProc
