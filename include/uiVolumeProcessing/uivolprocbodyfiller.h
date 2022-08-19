#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
