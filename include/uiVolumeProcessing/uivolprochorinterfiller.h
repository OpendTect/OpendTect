#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uivolumeprocessingmod.h"
#include "uivolprocstepdlg.h"
#include "volprochorinterfiller.h"

class uiIOObjSel;
class CtxtIOObj;


namespace VolProc
{

mExpClass(uiVolumeProcessing) uiHorInterFiller : public uiStepDialog
{ mODTextTranslationClass(uiHorInterFiller)
public:
    mDefaultFactoryInstanciationBase(
	    VolProc::HorInterFiller::sFactoryKeyword(),
	    VolProc::HorInterFiller::sFactoryDisplayName())
	    mDefaultFactoryInitClassImpl( uiStepDialog, createInstance )
protected:


				uiHorInterFiller(uiParent*,HorInterFiller*,
						 bool is2d);
				~uiHorInterFiller();

    static uiStepDialog*	createInstance(uiParent*,Step*,bool is2d);

    bool			acceptOK(CallBacker*) override;
    void			updateFlds(CallBacker*);

    HorInterFiller*		horinterfiller_;
    CtxtIOObj*			topctio_;
    CtxtIOObj*			bottomctio_;

    uiGenInput*			usetophorfld_;
    uiIOObjSel*			tophorfld_;
    uiGenInput*			topvalfld_;

    uiGenInput*			usebottomhorfld_;
    uiIOObjSel*			bottomhorfld_;

    uiGenInput*			usegradientfld_;

    uiGenInput*			gradientfld_;
    uiGenInput*			bottomvalfld_;

};

} // namespace VolProc
